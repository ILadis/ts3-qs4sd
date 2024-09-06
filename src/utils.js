
export function sleep(millis) {
  return new Promise(resolve => setTimeout(resolve, millis));
}

export function dispatch(action) {
  setTimeout(action, 0);
}

export function retry(action, times = 3) {
  return new Promise(async (resolve, reject) => {
    while (true) {
      try {
        let result = await action();
        return resolve(result);
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

export function debounce(action, millis = 200) {
  let handle = null;
  let queue = new Array();

  return enqueue;

  async function invoke() {
    if (queue.length == 0) {
      clearInterval(handle);
      handle = null;
    } else {
      let [thisArg, argArray] = queue[0];
      try {
        await action.apply(thisArg, argArray);
      } finally {
        queue.shift();
      }
    }
  }

  function enqueue() {
    queue.shift();
    queue.push([this, arguments]);

    if (handle == null) {
      handle = setInterval(invoke, millis);
      invoke();
    }
  }
}

export function generate() {
  let resolve = () => { };

  return { iterator, take };

  function* iterator() {
    while (true) {
      yield new Promise(next => resolve = next);
    }
  }

  function take(item) {
    resolve(item);
  }
}

export async function fətch(request) {
  let response = await fetch(request);
  if (!response.ok) {
    throw response;
  }
  return response;
}
