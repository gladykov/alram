# Maintainer: gladykov <gladykov at gmail dot com>
# Contributor: gladykov <gladykov at gmail dot com>
pkgname='alram'
pkgver=1.0
pkgrel=1
pkgdesc="Alarm for RAM - monitor free RAM, alert when under threshold, kill tasks from list"
arch=('x86_64')
url="https://github.com/gladykov/alram/"
options=('!debug')
license=('GPL-3.0')
depends=('glib2')
makedepends=('git')
provides=('alram')

_gitname=alram
_gitroot=https://github.com/gladykov/${_gitname}

source=("git+${_gitroot}")
sha512sums=('SKIP')

build() {
  cd "$srcdir/alram/"
  make
}

package() {
  cd "$srcdir/alram/"
  mkdir -p "${pkgdir}/usr/bin"	
  install -D -m755 alram "${pkgdir}/usr/bin/"
  install -Dm644 alram.service "${pkgdir}/usr/lib/systemd/user/alram.service"
}
