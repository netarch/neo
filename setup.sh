#!/bin/bash
#
# Set up the development environment
#

set -e
set -o nounset

cd "$(dirname ${BASH_SOURCE[0]})"

[ $UID -eq 0 ] && \
    (echo '[!] Please run this script without root privilege' >&2; exit 1)

# Dependencies needed for this script
depends=(git curl)


get_distro() {
    if test -f /etc/os-release; then
        # freedesktop.org and systemd
        . /etc/os-release
        DISTRO="$(echo $NAME | cut -f 1 -d ' ')"
    elif type lsb_release >/dev/null 2>&1; then
        # linuxbase.org
        DISTRO="$(lsb_release -si)"
    elif test -f /etc/lsb-release; then
        . /etc/lsb-release
        DISTRO="$DISTRIB_ID"
    elif test -f /etc/arch-release; then
        DISTRO="Arch"
    elif test -f /etc/debian_version; then
        # Older Debian, Ubuntu
        DISTRO="Debian"
    elif test -f /etc/SuSe-release; then
        # Older SuSE
        DISTRO="openSUSE"
    elif test -f /etc/fedora-release; then
        # Older Fedora
        DISTRO="Fedora"
    elif test -f /etc/redhat-release; then
        # Older Red Hat, CentOS
        DISTRO="CentOS"
    elif type uname >/dev/null 2>&1; then
        # Fall back to uname
        DISTRO="$(uname -s)"
    else
        echo '[!] unable to determine the distribution' >&2
        exit 1
    fi
}

aur_pkg_exists() {
    RETURN_CODE="$(curl -I "https://aur.archlinux.org/packages/$1" 2>/dev/null \
        | head -n1 | cut -d ' ' -f 2)"
    if [ "$RETURN_CODE" = "200" ]; then
        return 0    # package found
    elif [ "$RETURN_CODE" = "404" ]; then
        return 1    # package not found
    else
        echo "[!] AUR HTTP $RETURN_CODE for $1" >&2
        exit 1
    fi
    unset RETURN_CODE
}

aur_install() {
    TARGET="$1"
    shift
    if pacman -Q "$TARGET" &>/dev/null; then
        return 0
    fi
    if ! aur_pkg_exists "$TARGET"; then
        echo "[!] AUR package $TARGET not found" >&2
        return 1
    fi
    git clone "https://aur.archlinux.org/$TARGET.git"
    pushd "$TARGET"
    makepkg -srci --noconfirm $@
    popd
    rm -rf "$TARGET"
    unset TARGET
}

main() {
    get_distro

    if [ "$DISTRO" = "Arch" ]; then
        sudo pacman -S --needed --noconfirm --asdeps ${depends[@]}
        aur_install spin $@
    elif [ "$DISTRO" = "Ubuntu" ]; then
        sudo apt install -y ${depends[@]} spin
    else
        echo "[!] unsupported distribution: $DISTRO" >&2
        exit 1
    fi

    echo '[+] Finished!'
}


main $@

# vim: set ts=4 sw=4 et:
