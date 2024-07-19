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
        source /etc/os-release
        echo "$NAME" | cut -f 1 -d ' ' | tr '[:upper:]' '[:lower:]'
    elif type lsb_release >/dev/null 2>&1; then # linuxbase.org
        lsb_release -si | tr '[:upper:]' '[:lower:]'
    elif test -f /etc/lsb-release; then
        # shellcheck source=/dev/null
        source /etc/lsb-release
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
# Set up docker engine quick and dirty
#
get_docker() {
    curl -fsSL https://get.docker.com -o get-docker.sh
    sudo sh ./get-docker.sh
    rm -f ./get-docker.sh

    # Add the current user to group `docker` if they aren't in it already.
    if ! getent group docker | grep -qw "$USER"; then
        sudo gpasswd -a "$USER" docker
    fi
}

#
# Set up LLVM apt repository for Ubuntu
#
add_llvm_repo_for_ubuntu() {
    local llvm_version=18
    local code_name="${UBUNTU_CODENAME:-}"
    local signature="/etc/apt/trusted.gpg.d/apt.llvm.org.asc"
    local repo="deb http://apt.llvm.org/$code_name/  llvm-toolchain-$code_name-$llvm_version main"

    # check distribution
    if [[ "${DISTRO:-}" != "ubuntu" ]]; then
        die "The Linux distribution is not ubuntu: ${DISTRO:-}"
    fi
    if [[ -z "$code_name" ]]; then
        die "Unrecognizable ubuntu code name: $code_name"
    fi

    # check if the repo exists
    if ! wget -q --method=HEAD "http://apt.llvm.org/$code_name" &>/dev/null; then
        die "Failed to connect to http://apt.llvm.org/$code_name"
    fi

    # download GPG key once
    if [[ ! -f "$signature" ]]; then
        wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee "$signature"
    fi

    sudo add-apt-repository -y "$repo"
    sudo apt-get update -y -qq
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

    DISTRO="$(get_distro)"
    if [ "$DISTRO" = "arch" ]; then
        (makepkg_arch "$TARGET" "$@")
    else
        (makepkg_manual "$TARGET" "$@")
    fi
    rm -rf "$TARGET"
}

main() {
    DISTRO="$(get_distro)"

    if [[ "$DISTRO" == "arch" ]]; then
        if ! pacman -Q paru >/dev/null 2>&1; then
            aur_install paru --asdeps --needed --noconfirm --removemake
        fi

        script_deps=(base-devel curl git)
        style_deps=(clang yapf)
        bpf_deps=(elfutils libelf zlib binutils libcap clang llvm glibc lib32-glibc)
        dpdk_deps=(python meson ninja python-pyelftools numactl)
        experiment_deps=(python-matplotlib python-numpy python-pandas python-toml python-networkx)
        depends=("${script_deps[@]}" "${style_deps[@]}" "${bpf_deps[@]}"
            "${dpdk_deps[@]}" "${experiment_deps[@]}"
            gcc clang flex bison cmake ninja python-jinja pkgconf autoconf
            automake libtool
            zip unzip     # utils required by vcpkg
            linux-headers # openssl
            glibc         # libpthread
            gcc gcc-libs  # libstdc++
            libnl
            docker spin-git)

        paru -Sy --asdeps --needed --noconfirm --removemake "${depends[@]}"
        makepkg_arch neo-dev -srcfi --asdeps --noconfirm

    elif [[ "$DISTRO" == "ubuntu" ]]; then
        script_deps=(build-essential curl git)
        style_deps=(clang-format yapf3)
        bpf_deps=(elfutils libelf-dev zlib1g-dev binutils-dev libcap-dev clang llvm libc6-dev libc6-dev-i386)
        dpdk_deps=(python3 meson ninja-build python3-pyelftools libnuma-dev)
        experiment_deps=(python3-matplotlib python3-numpy python3-pandas python3-toml python3-networkx)
        depends=("${script_deps[@]}" "${style_deps[@]}" "${bpf_deps[@]}"
            "${dpdk_deps[@]}" "${experiment_deps[@]}"
            gcc g++ clang clang-tools flex bison libbison-dev cmake ninja-build
            python3-jinja2 pkgconf autoconf automake libtool
            zip unzip      # utils required by vcpkg
            linux-libc-dev # openssl
            libpthread-stubs0-dev
            libstdc++-12-dev # TODO: Try 13.
            libnl-3-200 libnl-3-dev libnl-genl-3-200 libnl-genl-3-dev)

        add_llvm_repo_for_ubuntu
        sudo apt update -y -qq
        sudo apt install -y -qq "${depends[@]}"

        if ! command -v docker >/dev/null 2>&1; then
            get_docker
        fi
        if ! command -v spin >/dev/null 2>&1; then
            aur_install spin-git
        fi

    else
        die "Unsupported distribution: $DISTRO"
    fi

    msg "Finished"
}

main "$@"

# vim: set ts=4 sw=4 et:
