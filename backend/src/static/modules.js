// from https://github.com/pixeldesu/moduleRaid

export function Modules(modules = { }) {
  this.modules = modules;
}

Modules.generateId = function() {
  return Math.random().toString(36).substring(7);
};

Modules.extractors = function*(cb) {
  var id = Modules.generateId();
  yield [
    [0],
    [(_, __, i) => {
      for (let mod in i.c) {
        cb(mod, i.c[mod].exports);
      }
    }]
  ];

  var id = Modules.generateId();
  yield [
    [1e3],
    {
      [id]: (_, __, i) => {
        for (let mod in i.c) {
          cb(mod, i.c[mod].exports);
        }
      },
    },
    [[id]]
  ];

  var id = Modules.generateId();
  yield [
    [id],
    { },
    (e) => {
      for (let mod in e.m) {
        cb(mod, e(mod));
      }
    }
  ];
};

Modules.fromWebpack = function(entrypoint = 'webpackJsonp') {
  const modules = new Modules();

  const webpack = window[entrypoint] || window.opener[entrypoint];
  const extractors = Modules.extractors((name, module) => modules.add(name, module));

  for (let extractor of extractors) {
    if (modules.count() > 0) break;

    try {
      if (typeof webpack === 'function') {
        webpack(...extractor);
      }
      
      if (typeof webpack.push === 'function') {
        webpack.push(extractor);
      }
    } catch (e) {
      // ignore, try next
    }
  }

  return modules;
};

Modules.prototype.add = function(name, module) {
  this.modules[name] = module;
};

Modules.prototype.find = function(query) {
  const results = new Array();
  
  for (let key in this.modules) {
    const module = this.modules[key];
    if (module === undefined || module === null) {
      continue;
    }

    let result = query(module);

    if (result === true) results.push(module);
    else if (!!result) results.push(result);
  }

  return results;
};

Modules.prototype.count = function() {
  return Object.keys(this.modules || { }).length;
};
