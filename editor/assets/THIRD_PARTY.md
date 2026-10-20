# Editor UI assets

## Lucide

Source: https://registry.npmjs.org/lucide-static/-/lucide-static-1.60.0.tgz
Version: 1.60.0. Upstream: https://github.com/lucide-icons/lucide
License: `LUCIDE_LICENSE.txt` (ISC, with MIT notices for Feather-derived icons).

The `lucide/` directory contains the subset used by Atlas. Root-level check, close, chevron, and link assets also use Lucide outlines. The package's generated SVG comment is removed and `currentColor` is resolved to `#ededed` for Qt's SVG loader. Geometry and strokes are unchanged. Attribution and permission notices are retained in the bundled license file.

## Geist

Source: https://registry.npmjs.org/geist/-/geist-1.8.1.tgz
Version: 1.8.1. Upstream: https://github.com/vercel/geist-font
License: `GEIST_LICENSE.txt` (SIL Open Font License 1.1).

`Geist-Variable.ttf` is the unmodified variable sans-serif font from the package. Both assets and licenses ship with the application. No network access is required to use them.

The pre-existing Manrope asset remains bundled because Graphite copies it into projects as its default content font. The editor UI uses Geist.
