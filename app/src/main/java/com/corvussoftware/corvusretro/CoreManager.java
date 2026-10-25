// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

/** One worker owns idle cores and transfers ownership to at most one player. */
final class CoreManager {
    interface Completion { void done(String error); }
    private static final HandlerThread THREAD;
    private static final Handler WORKER;
    private static final Handler UI = new Handler(Looper.getMainLooper());
    private static final Set<Runnable> observers = new HashSet<>();
    private static volatile int loaded = -1;
    private static NativeCore idle;
    private static Object owner;
    static {
        THREAD = new HandlerThread("CorvusCore");
        THREAD.start();
        WORKER = new Handler(THREAD.getLooper());
    }
    private CoreManager() {}
    static Handler worker() { return new Handler(THREAD.getLooper()); }
    static int loaded() { return loaded; }
    // Observers are added, removed and invoked only on the Android UI thread.
    static void observe(Runnable observer) { observers.add(observer); }
    static void forget(Runnable observer) { observers.remove(observer); }
    private static void publish(int system) {
        loaded = system;
        UI.post(() -> { for (Runnable observer : new ArrayList<>(observers)) observer.run(); });
    }
    private static void requireWorker() {
        if (Looper.myLooper() != THREAD.getLooper()) throw new IllegalStateException("File d'émulation requise");
    }
    private static void requireIdle() {
        if (owner != null) throw new IllegalStateException("Fermez le contenu en cours avant de changer de cœur.");
    }
    private static NativeCore prepare(int system) {
        requireWorker(); requireIdle();
        if (idle == null || loaded != system) {
            NativeCore replacement = new NativeCore(system);
            if (idle != null) idle.close();
            idle = replacement;
            publish(system);
        }
        return idle;
    }
    static void load(int system, Completion completion) {
        WORKER.post(() -> {
            String error = null;
            try { prepare(system); }
            catch (RuntimeException | OutOfMemoryError failure) { error = failure.getMessage(); if (error == null) error = failure.getClass().getSimpleName(); }
            String result = error;
            UI.post(() -> completion.done(result));
        });
    }
    static void unload(Completion completion) {
        WORKER.post(() -> {
            String error = null;
            try {
                requireIdle();
                if (idle != null) idle.close();
                idle = null;
                publish(-1);
            } catch (RuntimeException failure) { error = failure.getMessage(); }
            String result = error;
            UI.post(() -> completion.done(result));
        });
    }
    static NativeCore acquire(int system, Object session) {
        NativeCore result = prepare(system);
        idle = null;
        owner = session;
        return result;
    }
    static void release(Object session, NativeCore core) {
        requireWorker();
        if (owner != session) throw new IllegalStateException("Propriétaire de cœur incohérent");
        core.close();
        owner = null;
        publish(-1);
    }
}
