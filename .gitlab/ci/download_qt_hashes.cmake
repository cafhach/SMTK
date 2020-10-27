# Lines can be generated by doing:
#
#   sha256sum $files | awk '{ print "set(\"" $2 "_hash\" " $1 ")" }' >> $thisfile

if (msvc_year STREQUAL "2015")
  set("5.15.1-0-202009071110d3dcompiler_47-x64.7z_hash" a55cb8e4ef0a8271fd9937ca5d6305aa3994b178c675f4f39ecf0ee85b027d9d)
  set("5.15.1-0-202009071110opengl32sw-64-mesa_12_0_rc2.7z_hash" 3408f3bc754a51835171ace18be3b0b71a5384c29f03917daef59b939e11a728)
else ()
  set("5.15.1-0-202009071110d3dcompiler_47-x64.7z_hash" 76defcbef78e29832a5fca39607deac58419d49963fda3d1d61fd5a76c5de78b)
  set("5.15.1-0-202009071110opengl32sw-64-mesa_12_0_rc2.7z_hash" f4697dbe90fd5302a48558e22f4c9ce657d93c00247c24a1edb6d425303a0d80)
endif ()

set("5.15.1-0-202009071110qtbase-Windows-Windows_10-MSVC2015-Windows-Windows_10-X86_64.7z_hash" 5d0d2e71e3b00cf88ac4c616b4b314a7e73871f325512821f53c464cdfee961f)
set("5.15.1-0-202009071110qtsvg-Windows-Windows_10-MSVC2015-Windows-Windows_10-X86_64.7z_hash" 427e891531f775541b38f7fca3cdcb281af9ad4ef82b1b483a802b5ed211ea38)
set("5.15.1-0-202009071110qttools-Windows-Windows_10-MSVC2015-Windows-Windows_10-X86_64.7z_hash" 393630ea20cc5fa29e7987df21526586bcac23c3499fb9889d980758942f942e)
set("5.15.1-0-202009071110qtxmlpatterns-Windows-Windows_10-MSVC2015-Windows-Windows_10-X86_64.7z_hash" fc8c0b5776dee01fee943d13154400806acaef84d5b90ad682e3dcae967106ea)

set("5.15.1-0-202009071110qtbase-Windows-Windows_10-MSVC2019-Windows-Windows_10-X86_64.7z_hash" a5635124a135f383d9fb92bf628b018cff9f781addfd388926a367cda5b7cd38)
set("5.15.1-0-202009071110qtsvg-Windows-Windows_10-MSVC2019-Windows-Windows_10-X86_64.7z_hash" 9f344dafb9d00fadef4801a89e2e9d6f1599c8b25d3afd290a6c42f73ea16ed1)
set("5.15.1-0-202009071110qttools-Windows-Windows_10-MSVC2019-Windows-Windows_10-X86_64.7z_hash" 45b3debfc317f875d327e4506c0d211dc82ec92c1e9aa60e17b1a747ada22811)
set("5.15.1-0-202009071110qtxmlpatterns-Windows-Windows_10-MSVC2019-Windows-Windows_10-X86_64.7z_hash" dffcaf8a2028adb6621eb86ade6aa0bc8daa8961653a4a82841091b5165040ca)

set("5.15.1-0-202009071110qtbase-MacOS-MacOS_10_13-Clang-MacOS-MacOS_10_13-X86_64.7z_hash" df2813ce7c6cb4287abd7956cd1cb9d08312e4ac1208b6cb57af4df11b8ebba1)
set("5.15.1-0-202009071110qtsvg-MacOS-MacOS_10_13-Clang-MacOS-MacOS_10_13-X86_64.7z_hash" 5e624dd519dc67ef80ea60f83162dd49e9a0e912122ad049a0a83af914a97ea2)
set("5.15.1-0-202009071110qttools-MacOS-MacOS_10_13-Clang-MacOS-MacOS_10_13-X86_64.7z_hash" 94c5e98afa548bf56c7ccc29bccf727b75e2e90df98e83d22575d07f64359cda)
set("5.15.1-0-202009071110qtxmlpatterns-MacOS-MacOS_10_13-Clang-MacOS-MacOS_10_13-X86_64.7z_hash" fe01c55dfb073064fe24e1e77564951ccfa21042b91ff8bd3d629854d0b8a7b7)
