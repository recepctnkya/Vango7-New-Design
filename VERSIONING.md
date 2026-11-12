# Display7 sürüm yönetimi (makro / mikro)

| Kavram | Örnek | Anlam |
|--------|--------|--------|
| **Makro** | `v0.50` | Ürün çizgisi (`line` = `v0-50`) |
| **Mikro** | `v0.50-0001` | Anlamlı değişiklik takip |
| **SemVer** | `0.50.0` | CMake `project(VERSION …)` |

## Dosyalar

- `VERSION.json`, `changelog.json`
- `main/hexnet_version.h` — `HEXNET_DISP_RELEASE` / `HEXNET_DISP_MICRO`
- `scripts/Sync-VersionFiles.ps1`, `scripts/Bump-Version.ps1`
- `build.ps1` — Sync + `idf.py build`

```powershell
.\scripts\Bump-Version.ps1 -Bump micro -Notes "link status icons"
.\build.ps1
```

Otomatik mikro: `main/` + `components/hexnet` değişip **build** edilince artar.
- `idf.py build` / `flash` → CMake `hexnet_bump_if_changed` (Bump-IfChanged.ps1)
- `build.ps1` → aynı script, sonra idf.py build
Değişiklik yoksa bump yok. Cihaz Bilgisi → Yazılım = makro, Build = mikro.

Otomatik bump için `Desktop\tools\hexnet-version\HexnetVersion.psm1` gerekir.
Modül bulunamazsa build devam eder ve otomatik bump atlandığına dair uyarı verir.
