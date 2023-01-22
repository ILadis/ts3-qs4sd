
export function html(source) {
  let template = document.createElement('template');
  template.innerHTML = source[0];

  return document.importNode(template.content, true);
}

export function define(tag, base, template, ctor) {
  let proto = document.createElement(base).constructor;
  let options = { extends: base };

  let element = (class extends proto {
    constructor() {
      super();
      this.appendChild(template.cloneNode(true));
      this.setAttribute('is', tag);
      ctor?.call(this);
    }
  });

  customElements.define(tag, element, options);
  return element;
}

export function sleep(millis) {
  return new Promise(resolve => window.setTimeout(resolve, millis));
}

export function retry(action, times = 3) {
  return new Promise(async (resolve, reject) => {
    while (true) {
      try {
        return resolve(await action());
      } catch (error) {
        if (--times > 0) {
          await sleep(500);
        } else {
          return reject(error);
        }
      }
    }
  });
}
