
export function sleep(millis) {
  return new Promise(resolve => window.setTimeout(resolve, millis));
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

export async function fətch(request) {
  let response = await window.fetch(request);
  if (!response.ok) {
    throw response;
  }
  return response;
}
