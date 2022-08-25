import { Modules } from './modules';

let modules = Modules.fromWebpack();

export const [React] = modules.find(module => {
  if (typeof module !== 'object') return false;
  return !!module?.createElement;
});

export const [ReactDOM] = modules.find(module => {
  if (typeof module !== 'object') return false;
  return !!module?.findDOMNode;
});

export const [ReactRouter] = modules.find(module => {
  if (typeof module !== 'object') return false;

  for (let prop in module) {
    if (module[prop]?.computeRootMatch) {
      return true;
    }
  }

  return false;
});

export const [Router] = modules.find(module => {
  if (typeof module !== 'object') return false;

  for (let prop in module) {
    if (module[prop]?.Navigate && module[prop]?.NavigationManager) {
      return module[prop];
    }
  }

  return false;
});

export const [CommonUIModule] = modules.find(module => {
  if (typeof module!== 'object') return false;

  for (let prop in module) {
    if (module[prop]?.contextType && Object.keys(module).length > 60) {
      return new Modules(module);
    }
  }

  return false;
});

export const [ButtonItem] = CommonUIModule.find(module => {
  if (typeof module !== 'object') return false;

  const render = module.render?.toString();

  return render?.includes('"highlightOnFocus","childrenContainerWidth"')
      || render?.includes('childrenContainerWidth:"min"');
});

export const [PanelSection] = modules.find(module => {
  if (typeof module !== 'object') return false;

  for (let prop in module) {
    if (module[prop]?.toString()?.includes('.PanelSection')) {
      return module[prop];
    }
  }
  return false;
});

export const [PanelSectionRow] = modules.find(module => {
  if (typeof module !== 'object') return false;

  for (let prop in module) {
    if (module[prop]?.toString()?.includes('.PanelSection')) {
      for (let mod of Object.values(module)) {
        if (!mod?.toString()?.includes('.PanelSection')) {
          return mod;
        }
      }
    }
  }

  return false;
});
