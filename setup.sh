#!/bin/bash
#
# Set up the development environment
#

set -e
set -o nounset

cd "$(dirname ${BASH_SOURCE[0]})"

[ $UID -eq 0 ] && (echo '[!] Please run this script without root privilege' >&2; exit 1)


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

arch_install_yay() {
    pacman -Q yay &>/dev/null && return 0
    git clone https://aur.archlinux.org/yay.git
    pushd yay
    makepkg -srci --noconfirm
    popd
    rm -rf yay
}

main() {
    get_distro

    if [ "$DISTRO" = "Arch" ]; then
        arch_install_yay
        yay -S --needed --noconfirm spin $@
    elif [ "$DISTRO" = "Ubuntu" ]; then
        sudo apt install spin
    else
        echo "[!] unsupported distribution: $DISTRO" >&2
        exit 1
    fi
}


main $@

# vim: set ts=4 sw=4 et:
