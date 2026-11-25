#pragma once

#include "common.h"

// ─────────────────────────── BILINGUAL (TR / EN) ────────────────────────────
static inline int CurrentLang() {
    return (g_lang.load(std::memory_order_relaxed) == 1) ? 1 : 0;
}

static inline const wchar_t* LStr(const wchar_t* tr, const wchar_t* en) {
    return (CurrentLang() == 1) ? en : tr;
}

static inline int DetectSystemLanguage() {
    LANGID lid = GetUserDefaultUILanguage();
    return (PRIMARYLANGID(lid) == LANG_TURKISH) ? 0 : 1;
}

static inline void EnsureLanguageInitialized(Settings& s) {
    if (s.lang != 0 && s.lang != 1) {
        s.lang = DetectSystemLanguage();
    }
    g_lang.store(s.lang, std::memory_order_relaxed);
}
