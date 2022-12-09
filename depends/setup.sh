#!/bin/bash
#
# Set up the development environment.
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

[ $UID -eq 0 ] && die 'Please run this script without root privilege'

#
# Output a short name of the Linux distribution
#
get_distro() {
    if test -f /etc/os-release; then # freedesktop.org and systemd
        . /etc/os-release
        echo "$NAME" | cut -f 1 -d ' ' | tr '[:upper:]' '[:lower:]'
    elif type lsb_release >/dev/null 2>&1; then # linuxbase.org
        lsb_release -si | tr '[:upper:]' '[:lower:]'
    elif test -f /etc/lsb-release; then
        . /etc/lsb-release
        echo "$DISTRIB_ID" | tr '[:upper:]' '[:lower:]'
    elif test -f /etc/arch-release; then
        echo "arch"
    elif test -f /etc/debian_version; then
        # Older Debian, Ubuntu
        echo "debian"
    elif test -f /etc/SuSe-release; then
        # Older SuSE
        echo "opensuse"
    elif test -f /etc/fedora-release; then
        # Older Fedora
        echo "fedora"
    elif test -f /etc/redhat-release; then
        # Older Red Hat, CentOS
        echo "centos"
    elif type uname >/dev/null 2>&1; then
        # Fall back to uname
        uname -s
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

elementIn() {
    local e match="$1"
    shift
    for e in "$@"; do [[ "$e" == "$match" ]] && return 0; done
    return 1
}

#
# Build and install the package with PKGBUILD
#
makepkg_ubuntu() {
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
        if [[ "$target" == *.tar.* ]] && ! elementIn "$target" "${noextract[@]}"; then
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
    ("makepkg_$(get_distro)" "$TARGET" "$@")
    rm -rf "$TARGET"
}

main() {
    DISTRO="$(get_distro)"

    if [ "$DISTRO" = "arch" ]; then
        if ! pacman -Q paru >/dev/null 2>&1; then
            aur_install paru --asdeps --needed --noconfirm --removemake
        fi

        depends=(cmake clang spin-git libnet libnl pcapplusplus ipvsadm squid
                 python-toml bc yapf)
        non_local_depends=()

        for dep in "${depends[@]}"; do
            if [[ -d "$dep" ]]; then
                "makepkg_$DISTRO" "$dep" -srci --asdeps --noconfirm "$@"
            else
                non_local_depends+=("$dep")
            fi
        done

        if [[ ${#non_local_depends[@]} -ne 0 ]]; then
            paru -S --asdeps --needed --noconfirm --removemake \
                "${non_local_depends[@]}" "$@"
        fi

        "makepkg_$DISTRO" neo-dev -srci --asdeps --noconfirm "$@"

    elif [ "$DISTRO" = "ubuntu" ]; then
        script_depends=(build-essential curl git bison rsync)
        depends=(cmake pkg-config clang libpthread-stubs0-dev libnet1-dev
                 libnl-3-dev libnl-genl-3-dev libnet1 libnl-3-200
                 libnl-genl-3-200 ipvsadm squid python3-toml bc clang-format
                 yapf3 libpcap-dev libstdc++-static)
        aur_pkgs=(spin-git pcapplusplus)

        sudo apt update -y -qq
        sudo apt install -y -qq "${script_depends[@]}"
        sudo apt install -y -qq "${depends[@]}"
        for pkg in "${aur_pkgs[@]}"; do
            aur_install "$pkg"
        done

    else
        die "Unsupported distribution: $DISTRO"
    fi

    msg "Finished"
}


main "$@"

# vim: set ts=4 sw=4 et:
