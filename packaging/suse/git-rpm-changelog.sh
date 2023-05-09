#!/bin/bash

merge_commit() {
  git show --no-patch --format='%P' "$@" | head -1 | grep -q ' '
}

commit_meta() {
  git show --no-patch --format="* %cd %aN <%ae> - %H" --date=local "$@" | \
    head -1 | \
    sed -E 's/^(\* [a-z0-9 ]{9,10}) \d{2}:\d{2}:\d{2}/\1/i'
}

merge_meta() {
  local author=`git show --no-patch --format="%aN <%ae>" --date=local "$2" | head -1`
  git show --no-patch --format="* %cd @@AUTHOR@@ - %H" --date=local "$1" | \
    head -1 | \
    sed -E 's/^(\* [a-z0-9 ]{9,10}) \d{2}:\d{2}:\d{2}/\1/i; s/@@AUTHOR@@/'"$author"'/'
}

commit_mesg() {
  git log --format="- %s%d" --date=local "$@" # | cut 1-79
}

filter_vers() {
  perl -pe '
    s/^(\* [a-z0-9 ]{9,10}) \d{2}:\d{2}:\d{2}/$1/i;
    if (m/(?<= - )([0-9a-f]{40})$/) {
      $hash = $1; 
      $ver = `git describe --tags --match="v[0-9]*" --long --abbrev=8 $hash 2>/dev/null`;
      $ver =~ s/^v(.*)-(\d*)-g([0-9a-f]*)\n?/$1.$2+$3/;
      s/ - $hash$/ - $ver/ if $ver;
    }'
}

commit_vers() {
  git describe --tags --match="v[0-9]*" --long --abbrev=8 "$@" | sed -E 's/^v(.*)-([0-9]*)-g([0-9a-f]*)/\1.\2+\3/'
}

get_commits() {
  git log --format='%H' "$@"
}

run_one() {
  local hash="$1"
  if false; then
    git log --format="* %cd %aN <%ae> - %H%n" --date=local $hash^-1 | \
      head -1 | \
      sed -E 's/^(\* [A-Za-z0-9 ]{9,10}) [0-9]{2}:[0-9]{2}:[0-9]{2} (.*) - [0-9a-f]{40}$/\1 \2 - '$ver'-1/'
    git log --format="- %s%d" $hash^-1
  else
    if merge_commit $hash; then
      local branch=`git show --no-patch --format='%P' $hash | head -1 | cut -d ' ' -f 2`
      merge_meta $hash $branch
    else
      commit_meta $hash
    fi
    commit_mesg $hash^-1
  fi
}

run() {
  FIRST=1
  local hash=""
  for hash in `get_commits --first-parent "$@"`; do
    if [ $FIRST = 0 ]; then
      echo ""
    fi
    FIRST=0
    run_one $hash
  done | filter_vers
}

run "$@"
