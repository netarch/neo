#!/bin/bash
#
# Set up the environment for full emulation (with containerlab).
# https://containerlab.dev/
#

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
cd "$SCRIPT_DIR"

msg() {
    echo -e "[+] ${1-}" >&2
}

die() {
    echo -e "[!] ${1-}" >&2
    exit 1
}

if [[ $UID -eq 0 ]]; then
    die 'Please run this script without root privilege'
fi

#
# Output a short name (v:DISTRO) of the Linux distribution
#
get_distro() {
    export DISTRO

    if test -f /etc/os-release; then # freedesktop.org and systemd
        source /etc/os-release
        DISTRO="$(echo "$NAME" | cut -f 1 -d ' ' | tr '[:upper:]' '[:lower:]')"
    elif type lsb_release >/dev/null 2>&1; then # linuxbase.org
        DISTRO="$(lsb_release -si | tr '[:upper:]' '[:lower:]')"
    elif test -f /etc/lsb-release; then
        # shellcheck source=/dev/null
        source /etc/lsb-release
        DISTRO="$(echo "$DISTRIB_ID" | tr '[:upper:]' '[:lower:]')"
    elif test -f /etc/arch-release; then
        DISTRO="arch"
    elif test -f /etc/debian_version; then
        # Older Debian, Ubuntu
        DISTRO="debian"
    elif test -f /etc/SuSe-release; then
        # Older SuSE
        DISTRO="opensuse"
    elif test -f /etc/fedora-release; then
        # Older Fedora
        DISTRO="fedora"
    elif test -f /etc/redhat-release; then
        # Older Red Hat, CentOS
        DISTRO="centos"
    elif type uname >/dev/null 2>&1; then
        # Fall back to uname
        DISTRO="$(uname -s)"
    else
        die 'Unable to determine the distribution'
    fi
}

#
# Build and install the package with PKGBUILD
#
makepkg_arch() {
    TARGET="$1"
    shift
    msg "Building $TARGET..."
    pushd "$TARGET"
    makepkg -sri "$@"
    popd # "$TARGET"
}

element_in() {
    local e match="$1"
    shift
    for e in "$@"; do [[ "$e" == "$match" ]] && return 0; done
    return 1
}

#
# Build and install the package with PKGBUILD
#
makepkg_manual() {
    MAKEFLAGS="-j$(nproc)"
    export MAKEFLAGS
    [[ -z "${CFLAGS+x}" ]] && export CFLAGS=""
    [[ -z "${CXXFLAGS+x}" ]] && export CXXFLAGS=""

    TARGET="$1"
    shift
    msg "Building $TARGET..."
    pushd "$TARGET"
    # shellcheck disable=SC2016
    sed -i PKGBUILD \
        -e 's|\<python\> |python3 |g' \
        -e '/[[:space:]]*rm -rf .*\$pkgdir\>.*$/d'
    # shellcheck source=/dev/null
    source PKGBUILD
    srcdir="$(realpath src)"
    pkgdir=/
    mkdir -p "$srcdir"
    # prepare the sources
    i=0
    # shellcheck disable=SC2154
    for s in "${source[@]}"; do
        target=${s%%::*}
        url=${s#*::}
        if [[ "$target" == "$url" ]]; then
            target=$(basename "${url%%#*}" | sed 's/\.git$//')
        fi
        # fetch the source files if they do not exist already
        if [[ ! -e "$target" ]]; then
            # only support common tarballs and git sources
            if [[ "$url" == git+http* ]]; then
                # shellcheck disable=SC2001
                git clone "$(echo "${url%%#*}" | sed -e 's/^git+//')" "$target"
                # check out the corresponding revision if there is a fragment
                fragment=${url#*#}
                if [[ "$fragment" != "$url" ]]; then
                    pushd "$target"
                    git checkout "${fragment#*=}"
                    popd
                fi
            elif [[ "$url" == *.tar.* ]]; then
                curl -L "$url" -o "$target" >/dev/null 2>&1
            else
                die "Unsupported source URL $url"
            fi
        fi
        # create links in the src directory
        ln -sf "../$target" "$srcdir/$target"
        # extract tarballs if the target is not in noextract
        # shellcheck disable=SC2154
        if [[ "$target" == *.tar.* ]] &&
            ! element_in "$target" "${noextract[@]}"; then
            tar -C "$srcdir" -xf "$srcdir/$target"
        fi
        i=$((i + 1))
    done
    # execute the PKGBUILD functions
    pushd "$srcdir"
    [ "$(type -t prepare)" = "function" ] && prepare
    [ "$(type -t build)" = "function" ] && build
    [ "$(type -t check)" = "function" ] && check
    sudo bash -c "pkgdir=\"$pkgdir\"; srcdir=\"$srcdir\";
                  source \"$srcdir/../PKGBUILD\"; package"
    popd # "$srcdir"
    popd # "$TARGET"
}

#
# Build and install package from AUR
#
aur_install() {
    TARGET="$1"
    shift
    if [[ -d "$TARGET" ]]; then
        cd "$TARGET"
        git pull
        cd ..
    else
        git clone "https://aur.archlinux.org/$TARGET.git"
    fi

    if [[ "$DISTRO" == "arch" ]]; then
        (makepkg_arch "$TARGET" "$@")
    else
        (makepkg_manual "$TARGET" "$@")
    fi
    rm -rf "$TARGET"
}

main() {
    get_distro

    if [[ "$DISTRO" == "arch" ]]; then
        if ! pacman -Q paru >/dev/null 2>&1; then
            aur_install paru --asdeps --needed --noconfirm --removemake
        fi

        depends=(containerlab-bin)
        paru -Sy --asdeps --needed --noconfirm --removemake "${depends[@]}"

    elif [[ "$DISTRO" == "ubuntu" ]]; then
        bash -c "$(curl -sL https://get.containerlab.dev)"

    else
        die "Unsupported distribution: $DISTRO"
    fi

    msg "Finished"
}

main "$@"

# vim: set ts=4 sw=4 et:
