// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include <jni.h>
#include <corvuscore/bridge.h>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr std::size_t MAX_ROM = 32 * 1024 * 1024;
constexpr std::size_t MAX_STATE = 16 * 1024 * 1024;
struct Engine {
    std::unique_ptr<corvuscore_bridge, decltype(&corvuscore_destroy)> core{nullptr, corvuscore_destroy};
    std::vector<uint32_t> pixels;
    std::vector<int16_t> samples;
    uint64_t battery_generation = 0;
    bool battery_pending = false;
    bool loaded = false;
};
std::mutex mutex;
std::unordered_map<jlong, std::unique_ptr<Engine>> engines;
jlong next_id = 1;

void raise(JNIEnv* env, const char* type, const char* message) {
    if (env->ExceptionCheck()) return;
    jclass cls = env->FindClass(type);
    if (cls) env->ThrowNew(cls, message);
}
template<class F> auto guarded(JNIEnv* env, F operation) -> decltype(operation()) {
    using Result = decltype(operation());
    try {
        std::lock_guard lock(mutex);
        return operation();
    } catch (const std::bad_alloc&) {
        raise(env, "java/lang/OutOfMemoryError", "Mémoire insuffisante pour le cœur");
    } catch (const std::invalid_argument& error) {
        raise(env, "java/lang/IllegalArgumentException", error.what());
    } catch (const std::exception& error) {
        raise(env, "java/lang/IllegalStateException", error.what());
    }
    if constexpr (!std::is_void_v<Result>) return Result{};
}
Engine& engine(jlong id, bool loaded = true) {
    auto found = engines.find(id);
    if (found == engines.end()) throw std::invalid_argument("Session fermée ou inconnue");
    if (loaded && !found->second->loaded) throw std::invalid_argument("Aucun contenu chargé");
    return *found->second;
}
void require(int success) {
    if (!success) throw std::runtime_error(corvuscore_last_error());
}
std::vector<uint8_t> bytes(JNIEnv* env, jbyteArray array, std::size_t maximum, bool optional = false) {
    if (!array) {
        if (optional) return {};
        throw std::invalid_argument("Données absentes");
    }
    jsize count = env->GetArrayLength(array);
    if (count < 0 || static_cast<std::size_t>(count) > maximum) throw std::invalid_argument("Données trop volumineuses");
    std::vector<uint8_t> result(static_cast<std::size_t>(count));
    if (count) env->GetByteArrayRegion(array, 0, count, reinterpret_cast<jbyte*>(result.data()));
    if (env->ExceptionCheck()) throw std::runtime_error("Lecture du buffer Java impossible");
    return result;
}
jbyteArray java_bytes(JNIEnv* env, const std::vector<uint8_t>& data) {
    jbyteArray result = env->NewByteArray(static_cast<jsize>(data.size()));
    if (result && !data.empty()) env->SetByteArrayRegion(result, 0, static_cast<jsize>(data.size()), reinterpret_cast<const jbyte*>(data.data()));
    return result;
}
struct RomMapping {
    int fd = -1;
    void* data = MAP_FAILED;
    std::size_t size = 0;
    ~RomMapping() {
        if (data != MAP_FAILED) munmap(data, size);
        if (fd >= 0) close(fd);
    }
    void open_file(const char* path) {
        fd = open(path, O_RDONLY | O_CLOEXEC);
        struct stat status{};
        if (fd < 0 || fstat(fd, &status) != 0 || !S_ISREG(status.st_mode)) throw std::invalid_argument("Contenu illisible");
        if (status.st_size <= 0 || static_cast<uint64_t>(status.st_size) > MAX_ROM) throw std::invalid_argument("Taille de ROM invalide (maximum 32 Mio)");
        size = static_cast<std::size_t>(status.st_size);
        data = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) throw std::runtime_error("Lecture du contenu impossible");
    }
};
}

extern "C" {
JNIEXPORT jstring JNICALL Java_com_corvussoftware_corvusretro_NativeCore_version(JNIEnv* env, jclass) {
    return guarded(env, [&] { return env->NewStringUTF(corvuscore_version()); });
}
JNIEXPORT jlong JNICALL Java_com_corvussoftware_corvusretro_NativeCore_create(JNIEnv* env, jclass, jint system) {
    return guarded(env, [&]() -> jlong {
        if (corvuscore_api_version() != CORVUSCORE_API_VERSION) throw std::runtime_error("Version de l'API CorvusCore incompatible");
        if (system < CORVUSCORE_GB || system > CORVUSCORE_GBA) throw std::invalid_argument("Cœur inconnu");
        auto instance = std::make_unique<Engine>();
        instance->core.reset(corvuscore_create(system));
        require(instance->core != nullptr);
        if (next_id == std::numeric_limits<jlong>::max()) throw std::runtime_error("Identifiants de sessions épuisés");
        jlong id = next_id++;
        engines.emplace(id, std::move(instance));
        return id;
    });
}
JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_NativeCore_load(JNIEnv* env, jclass, jlong id, jstring path, jbyteArray battery) {
    guarded(env, [&] {
        auto& instance = engine(id, false);
        if (!path) throw std::invalid_argument("Contenu absent");
        const char* chars = env->GetStringUTFChars(path, nullptr);
        if (!chars) throw std::bad_alloc();
        std::string filename;
        try { filename = chars; } catch (...) { env->ReleaseStringUTFChars(path, chars); throw; }
        env->ReleaseStringUTFChars(path, chars);
        RomMapping rom;
        rom.open_file(filename.c_str());
        auto save = bytes(env, battery, 1024 * 1024, true);
        require(corvuscore_load(instance.core.get(), static_cast<const uint8_t*>(rom.data), rom.size, save.data(), save.size()));
        int width = corvuscore_video_width(instance.core.get());
        int height = corvuscore_video_height(instance.core.get());
        if (width <= 0 || height <= 0 || width > 1024 || height > 1024) throw std::runtime_error("Dimensions vidéo invalides");
        instance.pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
        instance.loaded = true;
    });
}
JNIEXPORT jintArray JNICALL Java_com_corvussoftware_corvusretro_NativeCore_info(JNIEnv* env, jclass, jlong id) {
    return guarded(env, [&] {
        auto& instance = engine(id);
        jint info[]{corvuscore_video_width(instance.core.get()), corvuscore_video_height(instance.core.get()),
            corvuscore_audio_rate(instance.core.get()), static_cast<jint>(corvuscore_video_fps(instance.core.get()) * 1000 + .5)};
        auto result = env->NewIntArray(4);
        if (result) env->SetIntArrayRegion(result, 0, 4, info);
        return result;
    });
}
JNIEXPORT jint JNICALL Java_com_corvussoftware_corvusretro_NativeCore_frame(JNIEnv* env, jclass, jlong id, jintArray pixels, jint buttons, jshortArray audio) {
    return guarded(env, [&]() -> jint {
        auto& instance = engine(id);
        if (!pixels || !audio || env->GetArrayLength(pixels) < static_cast<jsize>(instance.pixels.size()) || env->GetArrayLength(audio) < 4096)
            throw std::invalid_argument("Buffers vidéo/audio insuffisants");
        if (buttons < 0 || (buttons & ~1023)) throw std::invalid_argument("Boutons invalides");
        for (int button = 0; button < 10; ++button) require(corvuscore_set_button(instance.core.get(), button, (buttons & (1 << button)) != 0));
        require(corvuscore_run_frame(instance.core.get(), instance.pixels.data(), instance.pixels.size()));
        env->SetIntArrayRegion(pixels, 0, static_cast<jsize>(instance.pixels.size()), reinterpret_cast<const jint*>(instance.pixels.data()));
        if (env->ExceptionCheck()) return 0;
        instance.samples.resize(4096);
        auto count = corvuscore_read_audio(instance.core.get(), instance.samples.data(), instance.samples.size());
        if (count) env->SetShortArrayRegion(audio, 0, static_cast<jsize>(count), reinterpret_cast<const jshort*>(instance.samples.data()));
        return static_cast<jint>(count);
    });
}
JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_NativeCore_reset(JNIEnv* env, jclass, jlong id) {
    guarded(env, [&] { require(corvuscore_reset(engine(id).core.get())); });
}
JNIEXPORT jbyteArray JNICALL Java_com_corvussoftware_corvusretro_NativeCore_state(JNIEnv* env, jclass, jlong id) {
    return guarded(env, [&] {
        auto& instance = engine(id);
        auto size = corvuscore_state_size(instance.core.get());
        if (!size || size > MAX_STATE) throw std::runtime_error("État de sauvegarde invalide");
        std::vector<uint8_t> state(size);
        require(corvuscore_save_state(instance.core.get(), state.data(), state.size()));
        return java_bytes(env, state);
    });
}
JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_NativeCore_restore(JNIEnv* env, jclass, jlong id, jbyteArray data) {
    guarded(env, [&] {
        auto state = bytes(env, data, MAX_STATE);
        require(corvuscore_load_state(engine(id).core.get(), state.data(), state.size()));
    });
}
JNIEXPORT jbyteArray JNICALL Java_com_corvussoftware_corvusretro_NativeCore_battery(JNIEnv* env, jclass, jlong id) {
    return guarded(env, [&]() -> jbyteArray {
        auto& instance = engine(id);
        if (!corvuscore_battery_dirty(instance.core.get())) return nullptr;
        auto size = corvuscore_battery_size(instance.core.get());
        if (!size) return nullptr;
        if (size > 1024 * 1024) throw std::runtime_error("Sauvegarde batterie trop volumineuse");
        std::vector<uint8_t> data(size);
        require(corvuscore_copy_battery_generation(instance.core.get(), data.data(), data.size(), &instance.battery_generation));
        instance.battery_pending = true;
        return java_bytes(env, data);
    });
}
JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_NativeCore_acknowledgeBattery(JNIEnv* env, jclass, jlong id) {
    guarded(env, [&] {
        auto& instance = engine(id);
        if (instance.battery_pending) {
            require(corvuscore_acknowledge_battery(instance.core.get(), instance.battery_generation));
            instance.battery_pending = false;
        }
    });
}
JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_NativeCore_destroy(JNIEnv* env, jclass, jlong id) {
    guarded(env, [&] { engines.erase(id); });
}
}
