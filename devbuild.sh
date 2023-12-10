#!/bin/bash
set -e

function main {
  build::backend
  build::frontend
  build::upload
}

function build::backend {
  pushd backend
  make vendor
  make all
  popd
}

function build::frontend {
  pnpm install
  pnpm run build
  pnpm run build:zip
}

function build::upload {
  local name='Latest Devbuild'
  local description="$(concat \
    'This is the latest automatically built version of the plugin. It can manually installed by selecting the "Install Plugin"' \
    'from ZIP File" option in the DeckyLoader developer settings menu.'\
    '' \
    'Note: This version may be unstable since it may contain work in progress.')"

  local id=$(github::find_release "${name}")
  if [ -n "${id}" ]; then
    github::delete_release "${id}"
  fi

  id=$(github::create_release "${name}" "${description}" 'latest' 'master')
  github::upload_release_asset "${id}" 'plugin.zip'
}

function github::request {
  local method="${1}"
  local url="${2}"
  local token="${GITHUB_TOKEN}"

  shift 2
  curl -L \
    -X "${method}" \
    -H 'Accept: application/vnd.github+json' \
    -H "Authorization: Bearer ${token}" \
    -H 'X-GitHub-Api-Version: 2022-11-28' \
    -s "https://${url}" "${@}"
}

function github::find_release {
  local name="${1}"

  github::request GET 'api.github.com/repos/ILadis/ts3-qs4sd/releases' \
    | jq ".[] | select( .name == \"${name}\") | .id"
}

function github::create_release {
  local name="${1}"
  local description="${2}"
  local tag="${3}"
  local commit="${4}"

  jo "name=${name}" "body=${description}" "tag_name=${tag}" "target_commitish=${commit}" 'prerelease=true' \
    | github::request POST 'api.github.com/repos/ILadis/ts3-qs4sd/releases' --json @- \
    | jq '.id'
}

function github::delete_release {
  local id="${1}"

  github::request DELETE "api.github.com/repos/ILadis/ts3-qs4sd/releases/${id}"
}

function github::upload_release_asset {
  local id="${1}"
  local file="${2}"

  github::request POST "uploads.github.com/repos/ILadis/ts3-qs4sd/releases/${id}/assets?name=${file}" \
    -H 'Content-Type: application/octet-stream' \
    --data-binary "@${file}"
}

function concat {
  while (( $# )); do
    echo "${1}"
    shift
  done
}

main "${@}"
