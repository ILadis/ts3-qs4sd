
export function Locale() {
  this.translations = new Map();
  this.language = this.detectLanguage();
  this.fallback = false;
}

Locale.prototype.detectLanguage = async function() {
  let settings = window.SteamClient?.Settings;

  if ('GetCurrentLanguage' in settings) {
    var language = await settings.GetCurrentLanguage();
  }

  return language;
};

Locale.prototype.addTranslation = function(language, translations) {
  this.translations.set(language, translations);

  if (!this.fallback) {
    this.fallback = language;
  }
};

Locale.prototype.translate = async function(key, ...args) {
  let translations = this.translations.get(await this.language);
  let fallback = this.translations.get(this.fallback);

  let translation = key in translations ? translations[key] : fallback[key];

  switch (typeof translation) {
    case 'function': return translation(...args);
    case 'string':   return translation;
    case 'number':   return translation;
    default: return '';
  }
};
