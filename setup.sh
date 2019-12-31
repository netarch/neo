#!/bin/bash
#
# Set up the development environment
#

set -e
set -o nounset

SCRIPT_DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"
cd "$SCRIPT_DIR"

[ $UID -eq 0 ] && \
    (echo '[!] Please run this script without root privilege' >&2; exit 1)

# Dependencies needed for development
depends=(autoconf make spin-git astyle libnet)


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
        echo '[!] Unable to determine the distribution' >&2
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
    # Dependencies needed for this script (AUR)
    script_depends=(curl git)
    sudo pacman -S --needed --noconfirm --asdeps ${script_depends[@]}

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

aur_install_ubuntu() {
    # Dependencies needed for this script (AUR)
    script_depends=(curl git build-essential flex bison)
    sudo apt install -y -qq ${script_depends[@]}

    TARGET="$1"
    if ! aur_pkg_exists "$TARGET"; then
        echo "[!] AUR package $TARGET not found" >&2
        return 1
    fi
    git clone "https://aur.archlinux.org/$TARGET.git"
    pushd "$TARGET"
    source PKGBUILD
    srcdir="$(realpath src)"
    pkgdir=/
    set +o nounset
    if [ -n "$md5sums" ]; then
        checksum="md5sum"
        checksums=${md5sums[@]}
    elif [ -n "$sha1sums" ]; then
        checksum="sha1sum"
        checksums=${sha1sums[@]}
    elif [ -n "$sha224sums" ]; then
        checksum="sha224sum"
        checksums=${sha224sums[@]}
    elif [ -n "$sha256sums" ]; then
        checksum="sha256sum"
        checksums=${sha256sums[@]}
    elif [ -n "$sha384sums" ]; then
        checksum="sha384sum"
        checksums=${sha384sums[@]}
    elif [ -n "$sha512sums" ]; then
        checksum="sha512sum"
        checksums=${sha512sums[@]}
    else
        echo "[!] No checksum in PKGBUILD of $TARGET" >&2
        return 1
    fi
    set -o nounset
    mkdir -p "$srcdir"
    i=0
    for s in ${source[@]}; do
        # only support common tarball and git sources
        target=$(echo ${s%%::*})
        url=$(echo ${s#*::})
        if [[ $url == git+http* ]]; then
            if [[ $target == $url ]]; then
                target=$(basename $url | sed 's/\.git$//')
            fi
            git clone ${url#git+} $target
            pushd "$srcdir"
            ln -s "../$target" "$target"
            popd
        elif [[ $url == *.tar.gz ]]; then
            curl -LO "$s" &>/dev/null
            if [ "${checksums[$i]}" != "SKIP" ]; then
                echo "${checksums[$i]} $(basename $s)" | $checksum -c
            fi
            pushd "$srcdir"
            ln -s "../$(basename $s)" "$(basename $s)"
            tar xf $(basename $s)
            popd
        fi
        i=$((i + 1))
    done
    pushd "$srcdir"
    [ "$(type -t prepare)" = "function" ] && prepare
    [ "$(type -t build)" = "function" ] && build
    [ "$(type -t check)" = "function" ] && check
    sudo bash -c "pkgdir=\"$pkgdir\"; srcdir=\"$srcdir\";
                  source \"$srcdir/../PKGBUILD\"; package"
    popd
    popd
    rm -rf "$TARGET"
    unset TARGET
}

main() {
    get_distro

    if [ "$DISTRO" = "Arch" ]; then
        sudo pacman -Syy
        aur_install yay --needed --asdeps
        yay -S --needed --noconfirm ${depends[@]} $@

    elif [ "$DISTRO" = "Ubuntu" ]; then
        sudo apt update -y -qq
        deps=(build-essential libnet1-dev)
        for dep in ${depends[@]}; do
            if [ "$dep" != "spin-git" -a "$dep" != "libnet" ]; then
                deps+=("$dep")
            fi
        done
        sudo apt install -y -qq ${deps[@]}
        aur_install_ubuntu spin-git

    else
        echo "[!] Unsupported distribution: $DISTRO" >&2
        exit 1
    fi

    echo '[+] Finished!'
}


main $@

# vim: set ts=4 sw=4 et:
