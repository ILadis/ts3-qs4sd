
export function Locale() {
  this.translations = new Map();
  this.fallback = false;
}

Locale.prototype.detectLanguage = async function() {
  let settings = window.SteamClient?.Settings;

  if ('GetCurrentLanguage' in settings) {
    var language = await settings.GetCurrentLanguage();
  }

  this.language = language;
};

Locale.prototype.addTranslation = function(language, translations) {
  this.translations.set(language, translations);

  if (!this.fallback) {
    this.fallback = language;
  }
};

Locale.prototype.translate = function(key, ...args) {
  let language = this.language;

  if (!this.translations.has(language)) {
    language = this.fallback;
  }

  let translations = this.translations.get(language);
  let fallback = this.translations.get(this.fallback);

  let translation = key in translations ? translations[key] : fallback[key];

  switch (typeof translation) {
    case 'function': return translation(...args);
    case 'string':   return translation;
    case 'number':   return translation;
    default: return '';
  }
};
