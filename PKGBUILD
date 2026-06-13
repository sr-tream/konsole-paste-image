# Maintainer: sr_team <sr_team@sr.team>

pkgname=konsole-paste-image
pkgver=26.04.2
pkgrel=1
pkgdesc='Konsole plugin: Ctrl+V pastes clipboard images as hashed /tmp paths (bracketed-paste aware); Ctrl+Shift+V sends literal ^V'
arch=('x86_64' 'x86_64_v3' 'x86_64_v4' 'aarch64' 'armv7h' 'armv6h' 'riscv64')
url='https://github.com/sr-tream/konsole-paste-image'
license=('MIT')
depends=('konsole' 'qt6-base' 'kcoreaddons' 'ki18n' 'kxmlgui')
makedepends=('cmake' 'extra-cmake-modules' 'git')
source=("git+${url}.git")
sha256sums=('SKIP')

pkgver() {
    # Mirror the konsole release in [extra] so PluginManager's strict
    # version check (first 5 chars of "Version" must equal Konsole's
    # RELEASE_SERVICE_VERSION) passes on install.
    LANG=C pacman -Si konsole | awk -F': +' '/^Version/ {print $2; exit}' | cut -d- -f1
}

build() {
    cd "${srcdir}/${pkgname}"
    # pkgver() runs AFTER prepare(), so the konsole source clone and the
    # plugin-metadata version rewrite both have to happen here.
    if [[ ! -d konsole-src ]]; then
        git clone --depth=1 --branch "v${pkgver}" \
            https://invent.kde.org/utilities/konsole.git konsole-src
    fi
    sed -i "s/\"Version\": \"[^\"]*\"/\"Version\": \"${pkgver}\"/" \
        konsole_pasteimageplugin.json

    cmake -B build -S . \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DKONSOLE_SOURCE_DIR="${PWD}/konsole-src"
    cmake --build build
}

package() {
    cd "${srcdir}/${pkgname}"
    DESTDIR="${pkgdir}" cmake --install build
    install -Dm644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}
