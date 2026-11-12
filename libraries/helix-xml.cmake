# helix-xml: the declarative XML UI engine (MIT), compiled alongside LVGL.
# https://github.com/AndyEveritt/helix-xml (fork of prestonbrown/helix-xml)
#
# helix-xml has no build system of its own by design: compile its sources and
# give it LVGL's include paths. See libraries/helix-xml/README.md.

set(HELIX_XML_DIR ${LIBRARIES_DIR}/helix-xml)
set(HELIX_XML_COMPAT_DIR ${LIBRARIES_DIR}/helix-xml-compat)

file(GLOB HELIX_XML_SOURCES CONFIGURE_DEPENDS
     ${HELIX_XML_DIR}/src/xml/*.c
     ${HELIX_XML_DIR}/src/xml/parsers/*.c
     ${HELIX_XML_DIR}/src/libs/expat/*.c)

add_library(helix_xml STATIC ${HELIX_XML_SOURCES})
target_link_libraries(helix_xml PUBLIC lvgl)

# The public headers only include <lvgl.h>. The engine's internal headers (parser
# state, expat), which DuetScreen's custom widgets also use, reach LVGL's
# private headers through <lvgl-root>/src and the 9.6 include layout.
target_include_directories(helix_xml SYSTEM PUBLIC
                           ${HELIX_XML_DIR}
                           ${LIBRARIES_DIR}/lvgl/src
                           ${HELIX_XML_COMPAT_DIR})
target_include_directories(helix_xml PRIVATE ${HELIX_XML_DIR}/src)

# LVGL 9.5 compatibility for an engine written against 9.6.
target_compile_options(helix_xml PRIVATE -include ${HELIX_XML_COMPAT_DIR}/lvgl95_compat.h)

# Third-party C: don't apply DuetScreen's warning policy, and keep expat quiet.
target_compile_options(helix_xml PRIVATE -w)
