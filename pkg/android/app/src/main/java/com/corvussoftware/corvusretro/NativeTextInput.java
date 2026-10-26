// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.text.Editable;
import android.text.InputFilter;
import android.text.Selection;
import android.text.SpannableStringBuilder;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

/** Bridges only the Android keyboard; labels and fields are rendered by the C menu. */
final class NativeTextInput {
    private final MainActivity activity;
    private Editor editor;

    NativeTextInput(MainActivity activity) { this.activity = activity; }

    void open(int field, String initial, boolean secret) {
        activity.runOnUiThread(() -> {
            if (!activity.isAlive()) return;
            if (editor != null) editor.complete(false, false);
            editor = new Editor(field, initial, secret);
            activity.addContentView(editor, new ViewGroup.LayoutParams(1, 1));
            editor.requestFocus();
            Editor active = editor;
            active.post(() -> {
                if (!active.finished) ((InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE))
                        .showSoftInput(active, InputMethodManager.SHOW_IMPLICIT);
            });
        });
    }

    void cancel() {
        if (editor != null) editor.complete(false);
    }

    private final class Editor extends View {
        private final int field;
        private final boolean secret;
        private final SpannableStringBuilder value;
        private boolean finished;
        Editor(int field, String initial, boolean secret) {
            super(activity);
            this.field = field; this.secret = secret;
            value = new SpannableStringBuilder(initial == null ? "" : initial);
            value.setFilters(new InputFilter[] {new InputFilter.LengthFilter(secret ? 128 : 64)});
            Selection.setSelection(value, value.length());
            setFocusable(true); setFocusableInTouchMode(true);
            setImportantForAutofill(View.IMPORTANT_FOR_AUTOFILL_NO_EXCLUDE_DESCENDANTS);
        }
        @Override public boolean onCheckIsTextEditor() { return true; }
        @Override public InputConnection onCreateInputConnection(EditorInfo info) {
            info.inputType = InputType.TYPE_CLASS_TEXT | (secret
                    ? InputType.TYPE_TEXT_VARIATION_PASSWORD : InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
            info.imeOptions = EditorInfo.IME_ACTION_DONE | EditorInfo.IME_FLAG_NO_EXTRACT_UI
                    | EditorInfo.IME_FLAG_NO_PERSONALIZED_LEARNING;
            info.initialSelStart = info.initialSelEnd = value.length();
            return new BaseInputConnection(this, true) {
                @Override public Editable getEditable() { return value; }
                @Override public boolean commitText(CharSequence text, int cursor) {
                    boolean result = super.commitText(text, cursor); preview(); return result;
                }
                @Override public boolean setComposingText(CharSequence text, int cursor) {
                    boolean result = super.setComposingText(text, cursor); preview(); return result;
                }
                @Override public boolean deleteSurroundingText(int before, int after) {
                    boolean result = super.deleteSurroundingText(before, after); preview(); return result;
                }
                @Override public boolean performEditorAction(int action) { complete(true); return true; }
                @Override public boolean sendKeyEvent(KeyEvent event) {
                    if (event.getKeyCode() == KeyEvent.KEYCODE_ENTER && event.getAction() == KeyEvent.ACTION_UP) {
                        complete(true); return true;
                    }
                    return super.sendKeyEvent(event);
                }
            };
        }
        private void preview() { if (!finished) MainActivity.textInputResult(-field, value.toString(), true); }
        @Override public boolean onKeyDown(int code, KeyEvent event) {
            if (code == KeyEvent.KEYCODE_DEL) {
                int start = Math.max(0, Selection.getSelectionStart(value));
                int end = Math.max(start, Selection.getSelectionEnd(value));
                if (start != end) value.delete(start, end);
                else if (start > 0) value.delete(Character.offsetByCodePoints(value, start, -1), start);
                preview(); return true;
            }
            if (code == KeyEvent.KEYCODE_ENTER) { complete(true); return true; }
            int unicode = event.getUnicodeChar();
            if (unicode > 0 && !Character.isISOControl(unicode)) {
                int start = Math.max(0, Selection.getSelectionStart(value));
                int end = Math.max(start, Selection.getSelectionEnd(value));
                value.replace(start, end, new String(Character.toChars(unicode)));
                Selection.setSelection(value, value.length());
                preview(); return true;
            }
            return super.onKeyDown(code, event);
        }
        @Override public boolean onKeyPreIme(int code, KeyEvent event) {
            if (code == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_UP) {
                complete(false); return true;
            }
            return super.onKeyPreIme(code, event);
        }
        void complete(boolean accepted) { complete(accepted, true); }
        void complete(boolean accepted, boolean notify) {
            if (finished) return;
            finished = true;
            String text = accepted ? value.toString() : "";
            value.clear();
            ((InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE))
                    .hideSoftInputFromWindow(getWindowToken(), 0);
            if (getParent() instanceof ViewGroup) ((ViewGroup) getParent()).removeView(this);
            if (editor == this) editor = null;
            activity.getWindow().getDecorView().requestFocus();
            if (notify) MainActivity.textInputResult(field, text, accepted);
        }
    }
}
