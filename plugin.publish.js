#!/bin/node

import process from 'process';
import fs from 'fs';

const token = process.env['GITHUB_TOKEN'];
const client = new Github(token);

const fileName = 'plugin.zip';
const repoName = 'ILadis/ts3-qs4sd';
const releaseInfo = {
  'name': 'Latest Devbuild',
  'body': ''
    + 'This is the latest automatically built version of the plugin. It can be manually installed by selecting "Install Plugin \n'
    + 'from ZIP File" in the DeckyLoader developer settings menu.\n'
    + '\n'
    + 'Note: This version should be considered unstable since it may contain work in progress.',
  'tag_name': 'latest',
  'target_commitish': 'master',
  'prerelease': true,
};

const releases = await client.request({
  method: 'GET',
  path: `https://api.github.com/repos/${repoName}/releases`
});

var releaseId = releases.find(release => release.name == releaseInfo.name)?.id;
if (releaseId) {
  await client.request({
    method: 'DELETE',
    path: `https://api.github.com/repos/${repoName}/releases/${releaseId}`
  });

  await client.request({
    method: 'DELETE',
    path: `https://api.github.com/repos/${repoName}/git/refs/tags/latest`
  });
}

const release = await client.request({
  method: 'POST',
  path: `https://api.github.com/repos/${repoName}/releases`,
  headers: {
    'Content-Type': 'application/json'
  },
  body: JSON.stringify(releaseInfo),
});

var releaseId = release.id;

const stream = fs.createReadStream(fileName);
const stats = fs.statSync(fileName);

await client.request({
  method: 'POST',
  path: `https://uploads.github.com/repos/${repoName}/releases/${releaseId}/assets?name=${fileName}`,
  headers: {
    'Content-Type': 'application/octet-stream',
    'Content-Length': stats.size,
  },
  body: stream,
});

function Github(token) {
  return { request };

  async function request({ method, path, headers, body }) {
    var headers = new Headers(headers);
    headers.append('Accept', 'application/vnd.github+json');
    headers.append('Authorization', `Bearer ${token}`);

    const request = new Request(path, { method, body, headers, duplex: 'half' });
    const response = await fetch(request);

    var body = await response.text();
    if (!response.ok) {
      throw new Error(body);
    }

    if (body.length > 0) {
      return JSON.parse(body);
    }
  }
}
