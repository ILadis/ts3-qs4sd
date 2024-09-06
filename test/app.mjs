
import Assert from 'node:assert';
import { describe, it, mock } from 'node:test';
import { createElement as $ } from 'react';
import ReactTestRenderer from 'react-test-renderer';

describe('App', async () => {

  mock.module('decky-frontend-lib', {
    namedExports: {
      Field: mockComponent('Field'),
      Focusable: mockComponent('Focusable'),
      TextField: mockComponent('TextField'),
      Dropdown: mockComponent('Dropdown'),
      SliderField: mockComponent('SliderField'),
      PanelSection: mockComponent('PanelSection'),
      PanelSectionRow: mockComponent('PanelSectionRow'),
      ModalRoot: mockComponent('ModalRoot'),
      DialogBody: mockComponent('DialogBody'),
      DialogButton: mockComponent('DialogButton'),
      DialogHeader: mockComponent('DialogHeader'),
      DialogFooter: mockComponent('DialogFooter'),
      showModal: () => {},
      definePlugin: () => {},
    }
  });

  // fixtures
  const installedStatus = { installed: true };

  const nonMutedSelf = { id: 3, muted: { input: false, output: false } };

  const connectedServer = { status: 1, name: 'My TeamSpeak Server' };
  const disconnectedServer = { status: 0 };

  const bookmark1 = { name: 'My TeamSpeak Server', uuid: 'b5a77db9-a1e5-46d9-b297-3d555812c697' };
  const bookmark2 = { name: 'Another TeamSpeak Server', uuid: '2bdd7fea-1358-4f71-8064-96243916381b' };

  it('should display available bookmarks if not connected to any server', async () => {
    const { App } = await import('../src/app.js');
    const { TS3QuickAccessPanel, TS3BookmarkList } = await import('../src/components.js');

    // arrange
    const awaitListener = newPromise();

    const client = {
      start: () => Promise.resolve(),
      getStatus: () => Promise.resolve(installedStatus),
      getServer: () => Promise.resolve(disconnectedServer),
      getBookmarks: function*() {
        yield Promise.resolve(bookmark1);
        yield Promise.resolve(bookmark2);
      },
      listenEvents: function*() {
        yield awaitListener.resolve({ });
      }
    };

    // act
    const renderer = ReactTestRenderer.create($(App, { client }));
    await ReactTestRenderer.act(() => awaitListener);

    // assert
    const panel = renderer.root.findByType(TS3QuickAccessPanel);
    Assert.strictEqual(panel.props.content, 'bookmarks');

    const bookmarkList = renderer.root.findByType(TS3BookmarkList);
    Assert.strictEqual(bookmarkList.props.bookmarks[0], bookmark1);
    Assert.strictEqual(bookmarkList.props.bookmarks[1], bookmark2);
  });

  it('should attempt to connect to server if bookmark is clicked', async () => {
    const { App } = await import('../src/app.js');

    // arrange
    const capture = captureInvocation();
    const awaitListener = newPromise();

    const client = {
      connect: capture,
      start: () => Promise.resolve(),
      getStatus: () => Promise.resolve(installedStatus),
      getServer: () => Promise.resolve(disconnectedServer),
      getBookmarks: function*() {
        yield Promise.resolve(bookmark1);
        yield Promise.resolve(bookmark2);
      },
      listenEvents: function*() {
        yield awaitListener.resolve({ });
      }
    };

    // act
    const renderer = ReactTestRenderer.create($(App, { client }));
    await ReactTestRenderer.act(() => awaitListener);

    const field = renderer.root.findByProps({ label: bookmark2.name });
    await field.props.onClick();

    // assert
    Assert.strictEqual(capture.invocations.length, 1);

    const [connected] = capture.invocations[0];
    Assert.strictEqual(connected, bookmark2.uuid);
  });

  it('should display name of connected server in dashboard', async () => {
    const { App } = await import('../src/app.js');
    const { TS3Dashboard } = await import('../src/components.js');

    // arrange
    const awaitListener = newPromise();

    const client = {
      start: () => Promise.resolve(),
      getStatus: () => Promise.resolve({ installed: true }),
      getServer: () => Promise.resolve(connectedServer),
      getSelf: () => Promise.resolve(nonMutedSelf),
      getAudioInputs: function*() { },
      listChannels: () => Promise.resolve([]),
      moveBrowser: () => Promise.resolve(),
      browseChannels: () => Promise.resolve([]),
      listenEvents: function*() {
        yield awaitListener.resolve({ });
      }
    };

    // act
    const renderer = ReactTestRenderer.create($(App, { client }));
    await ReactTestRenderer.act(() => awaitListener);

    // assert
    const dashboard = renderer.root.findByType(TS3Dashboard);
    const panel = dashboard.find(node => 'title' in node.props);

    Assert.strictEqual(panel.props.title, connectedServer.name);
  });

  it('should toggle input mute state when action button is clicked', async () => {
    const { App } = await import('../src/app.js');
    const { TS3DashboardActions, TS3IconButton } = await import('../src/components.js');

    // arrange
    const capture = captureInvocation();
    const awaitListener = newPromise();

    const client = {
      muteSelf: capture,
      start: () => Promise.resolve(),
      getStatus: () => Promise.resolve({ installed: true }),
      getServer: () => Promise.resolve(connectedServer),
      getSelf: () => Promise.resolve(nonMutedSelf),
      getAudioInputs: function*() { },
      listChannels: () => Promise.resolve([]),
      moveBrowser: () => Promise.resolve(),
      browseChannels: () => Promise.resolve([]),
      listenEvents: function*() {
        yield awaitListener.resolve({ });
      }
    };

    // act
    const renderer = ReactTestRenderer.create($(App, { client }));
    await ReactTestRenderer.act(() => awaitListener);

    const actions = renderer.root.findByType(TS3DashboardActions);
    const buttons = actions.findAllByType(TS3IconButton);
    await buttons[0].props.onClick();

    // assert
    Assert.strictEqual(capture.invocations.length, 1);

    const [id, device, state] = capture.invocations[0];
    Assert.strictEqual(id, nonMutedSelf.id);
    Assert.strictEqual(device, 'input');
    Assert.strictEqual(state, true);
  });

  function mockComponent(name) {
    const mock = (props) => $('div', props, props.children);
    Object.defineProperty(mock, 'name', { value: name });
    return mock;
  }

  function newPromise() {
    let resolve, promise = new Promise(r => resolve = r);
    promise.resolve = (v) => (resolve(v), promise);
    return promise;
  }

  function captureInvocation() {
    const invocations = new Array();
    const capture = (...args) => invocations.push(args);
    capture.invocations = invocations;
    return capture;
  }
});
