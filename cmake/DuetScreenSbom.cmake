# ----------------------------------------------------------------------------
# SPDX SBOM generation
# ----------------------------------------------------------------------------
# CMake 4.3 added install(SBOM), which walks the link closure of an install
# export set and emits an SPDX 3.0.1 JSON-LD document describing the shipped
# artifacts and everything they pull in.
#
# In 4.3 and 4.4 the commands are gated behind CMAKE_EXPERIMENTAL_GENERATE_SBOM,
# which CMake only honours when it holds the UUID belonging to the exact
# release in use. The gate was dropped after 4.4
# (Help/release/dev/sbom.rst on CMake master), so from 4.5 onwards the commands
# are available unconditionally and the variable must not be set.
#
# When the cmake pin in requirements.txt moves to a new feature release, check
# Help/dev/experimental.rst in that release: if it still has a "Software Bill
# Of Materials" section, add its UUID to the table below.

option(DUETSCREEN_GENERATE_SBOM
       "Generate an SPDX SBOM for the installed artifacts" OFF)

if(NOT DUETSCREEN_GENERATE_SBOM)
  return()
endif()

if(CMAKE_VERSION VERSION_LESS "4.3")
  message(
    FATAL_ERROR
      "DUETSCREEN_GENERATE_SBOM is ON but install(SBOM) needs CMake 4.3 or "
      "newer and this is CMake ${CMAKE_VERSION}. The pinned version lives in "
      "requirements.txt; the presets run ${CMAKE_SOURCE_DIR}/env/bin/cmake "
      "from that virtualenv.")
endif()

set(_duetscreen_sbom_gate_4.3 "2d856d6d-53e8-488b-a17f-d486d2cac317")
set(_duetscreen_sbom_gate_4.4 "2d856d6d-53e8-488b-a17f-d486d2cac317")

set(_duetscreen_sbom_gate_key
    "_duetscreen_sbom_gate_${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")

if(DEFINED ${_duetscreen_sbom_gate_key})
  set(CMAKE_EXPERIMENTAL_GENERATE_SBOM "${${_duetscreen_sbom_gate_key}}")
elseif(CMAKE_VERSION VERSION_LESS "4.5")
  message(
    FATAL_ERROR
      "No SBOM feature gate is known for CMake ${CMAKE_VERSION}. Copy the "
      "'Software Bill Of Materials' UUID from Help/dev/experimental.rst in "
      "that release into cmake/DuetScreenSbom.cmake.")
endif()

# ----------------------------------------------------------------------------
# Metadata helpers
# ----------------------------------------------------------------------------
# Reduce a git tag or describe output to the plain numeric version SPDX wants:
# `v1.3.4` and `v9.3.0-1183-ga2d39cecd` both become usable versions.
function(_duetscreen_sbom_numeric_version text out_var)
  if("${text}" MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(${out_var} "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}"
        PARENT_SCOPE)
  elseif("${text}" MATCHES "([0-9]+)\\.([0-9]+)")
    set(${out_var} "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}" PARENT_SCOPE)
  else()
    set(${out_var} "" PARENT_SCOPE)
  endif()
endfunction()

# Strip the `.git` suffix and any trailing slash from a clone URL so it is
# presentable as a homepage.
function(_duetscreen_sbom_homepage_url url out_var)
  string(REGEX REPLACE "\\.git$" "" url "${url}")
  string(REGEX REPLACE "/+$" "" url "${url}")
  set(${out_var} "${url}" PARENT_SCOPE)
endfunction()

# ----------------------------------------------------------------------------
# Project metadata
# ----------------------------------------------------------------------------
# An SBOM version has to be a plain numeric version, so derive one from the
# most recent release tag instead of the full `git describe` string that
# version.h carries. Tags look like `v1.0.0-rc3`; only `1.0.0` is usable here.
set(DUETSCREEN_SBOM_VERSION "0.0.0")
find_package(Git QUIET)
if(GIT_EXECUTABLE)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --abbrev=0 --match=v*
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE _duetscreen_sbom_tag
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  _duetscreen_sbom_numeric_version("${_duetscreen_sbom_tag}" _duetscreen_sbom_tag_version)
  if(_duetscreen_sbom_tag_version)
    set(DUETSCREEN_SBOM_VERSION "${_duetscreen_sbom_tag_version}")
  endif()
endif()

# The repository ships the GPLv3 text without a trailing "or any later
# version" grant, so default to the conservative reading of it and leave the
# expression overridable.
set(DUETSCREEN_SBOM_LICENSE "GPL-3.0-only"
    CACHE STRING "SPDX license expression recorded in the generated SBOM")

set(DUETSCREEN_SBOM_HOMEPAGE "https://github.com/Duet3D/DuetScreen")

# Where the documents land inside the install prefix.
include(GNUInstallDirs)
set(DUETSCREEN_SBOM_DESTINATION "${CMAKE_INSTALL_DATADIR}/DuetScreen/sbom")

# ----------------------------------------------------------------------------
# Dependency metadata
# ----------------------------------------------------------------------------
# install(SBOM) refuses to emit a document it cannot fully attribute: every
# in-build target reachable through the link closure must either be exported
# with a NAMESPACE or be covered by an SBOM of its own. Metadata is per-SBOM
# rather than per-target, so each third-party library gets its own document;
# lumping them into the DuetScreen one would stamp every dependency with
# DuetScreen's version, homepage and license.
#
# Only the license is stated here. Licenses are not machine-readable -- nothing
# upstream sets the SPDX_LICENSE property, so each one was read off a LICENSE
# file by hand -- but versions and origins are, and restating them would let
# the SBOM drift silently the moment a dependency is bumped. They are therefore
# always derived:
#
#   FETCHCONTENT  from the DUETSCREEN_<NAME>_GIT_TAG / _URL variables set next
#                 to the matching FetchContent_Declare in libraries/, so the
#                 bump site and the SBOM are the same line
#   SUBMODULE     from `git describe` and the remote URL of the checkout, so it
#                 follows the commit actually being compiled

# _duetscreen_sbom_declare(<target> LICENSE <spdx-expression>
#                          FETCHCONTENT <name> | SUBMODULE <path> |
#                          VERSION <v> HOMEPAGE_URL <url>)
function(_duetscreen_sbom_declare target)
  set(_one_value LICENSE FETCHCONTENT SUBMODULE VERSION HOMEPAGE_URL)
  cmake_parse_arguments(PARSE_ARGV 1 _arg "" "${_one_value}" "")

  if(NOT _arg_LICENSE)
    message(FATAL_ERROR "SBOM metadata for '${target}' has no LICENSE.")
  endif()

  set(_description "")

  if(_arg_FETCHCONTENT)
    string(TOUPPER "${_arg_FETCHCONTENT}" _upper)
    if(NOT DEFINED DUETSCREEN_${_upper}_GIT_TAG
       OR NOT DEFINED DUETSCREEN_${_upper}_URL)
      message(
        FATAL_ERROR
          "SBOM metadata for '${target}' reads DUETSCREEN_${_upper}_GIT_TAG "
          "and DUETSCREEN_${_upper}_URL, but they are not set. Define them "
          "next to the FetchContent_Declare(${_arg_FETCHCONTENT}) call in "
          "libraries/ and pass them to GIT_TAG and GIT_REPOSITORY, so the tag "
          "that is fetched is the tag the SBOM reports.")
    endif()
    _duetscreen_sbom_numeric_version("${DUETSCREEN_${_upper}_GIT_TAG}" _version)
    if(NOT _version)
      message(
        FATAL_ERROR
          "DUETSCREEN_${_upper}_GIT_TAG is "
          "'${DUETSCREEN_${_upper}_GIT_TAG}', which has no numeric version in "
          "it for the SBOM to report. Pass VERSION to "
          "_duetscreen_sbom_declare(${target}) explicitly instead.")
    endif()
    _duetscreen_sbom_homepage_url("${DUETSCREEN_${_upper}_URL}" _homepage)

  elseif(_arg_SUBMODULE)
    set(_path "${PROJECT_SOURCE_DIR}/${_arg_SUBMODULE}")
    if(NOT GIT_EXECUTABLE)
      message(
        FATAL_ERROR
          "SBOM metadata for '${target}' comes from the ${_arg_SUBMODULE} "
          "submodule checkout, which needs git. Pass VERSION and "
          "HOMEPAGE_URL to _duetscreen_sbom_declare(${target}) explicitly if "
          "building without it.")
    endif()
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" describe --tags --match=v* --always
      WORKING_DIRECTORY "${_path}"
      OUTPUT_VARIABLE _describe
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" config --get remote.origin.url
      WORKING_DIRECTORY "${_path}"
      OUTPUT_VARIABLE _origin
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
    _duetscreen_sbom_numeric_version("${_describe}" _version)
    if(NOT _version OR NOT _origin)
      message(
        FATAL_ERROR
          "Could not read a version and origin for '${target}' from the "
          "${_arg_SUBMODULE} submodule (git describe gave '${_describe}', "
          "origin gave '${_origin}'). Run `git submodule update --init` or "
          "pass VERSION and HOMEPAGE_URL explicitly.")
    endif()
    _duetscreen_sbom_homepage_url("${_origin}" _homepage)
    # The numeric version loses the commit count and hash, and these forks run
    # well ahead of their nearest tag, so keep the exact checkout on record.
    set(_description "Submodule checkout ${_describe}")

  else()
    if(NOT _arg_VERSION OR NOT _arg_HOMEPAGE_URL)
      message(
        FATAL_ERROR
          "SBOM metadata for '${target}' needs FETCHCONTENT, SUBMODULE, or "
          "both VERSION and HOMEPAGE_URL.")
    endif()
    set(_version "${_arg_VERSION}")
    set(_homepage "${_arg_HOMEPAGE_URL}")
  endif()

  set(_duetscreen_sbom_${target}_VERSION "${_version}" PARENT_SCOPE)
  set(_duetscreen_sbom_${target}_LICENSE "${_arg_LICENSE}" PARENT_SCOPE)
  set(_duetscreen_sbom_${target}_HOMEPAGE "${_homepage}" PARENT_SCOPE)
  set(_duetscreen_sbom_${target}_DESCRIPTION "${_description}" PARENT_SCOPE)
endfunction()

# First-party targets. These all share the DuetScreen document, so they carry
# the project's own version, license and homepage.
set(_duetscreen_sbom_first_party
    DuetScreen
    DuetScreen.lib
    DuetScreen.themes
    lvgl_tracy)

foreach(_target IN LISTS _duetscreen_sbom_first_party)
  _duetscreen_sbom_declare(
    ${_target}
    VERSION "${DUETSCREEN_SBOM_VERSION}"
    LICENSE "${DUETSCREEN_SBOM_LICENSE}"
    HOMEPAGE_URL "${DUETSCREEN_SBOM_HOMEPAGE}")
endforeach()

# Third-party targets, each of which gets its own document. Note that the
# target name does not always match the name it is fetched or vendored under.
_duetscreen_sbom_declare(lvgl          SUBMODULE    libraries/lvgl  LICENSE MIT)
_duetscreen_sbom_declare(TracyClient   SUBMODULE    libraries/tracy LICENSE BSD-3-Clause)
_duetscreen_sbom_declare(hv_static     FETCHCONTENT hv              LICENSE BSD-3-Clause)
_duetscreen_sbom_declare(nlohmann_json FETCHCONTENT nlohmann_json   LICENSE MIT)
_duetscreen_sbom_declare(nameof        FETCHCONTENT nameof          LICENSE MIT)
_duetscreen_sbom_declare(magic_enum    FETCHCONTENT magic_enum      LICENSE MIT)
_duetscreen_sbom_declare(spdlog        FETCHCONTENT spdlog          LICENSE MIT)
_duetscreen_sbom_declare(fmt           FETCHCONTENT fmt             LICENSE MIT)

# ----------------------------------------------------------------------------
# Link closure discovery
# ----------------------------------------------------------------------------
# Walking the graph instead of hard-coding the target list keeps the SBOM
# complete when a library is added, removed, or switched off by an option.
function(_duetscreen_sbom_collect_deps target out_var)
  set(_seen "")
  set(_queue "${target}")

  while(_queue)
    list(POP_FRONT _queue _current)

    get_target_property(_aliased "${_current}" ALIASED_TARGET)
    if(_aliased)
      set(_current "${_aliased}")
    endif()

    if(_current IN_LIST _seen)
      continue()
    endif()
    list(APPEND _seen "${_current}")

    set(_candidates "")
    foreach(_property IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
      get_target_property(_values "${_current}" ${_property})
      if(_values)
        list(APPEND _candidates ${_values})
      endif()
    endforeach()

    foreach(_candidate IN LISTS _candidates)
      # PRIVATE link libraries are recorded in the interface as
      # $<LINK_ONLY:...>, so unwrap those. Everything else that is not a plain
      # target name -- raw linker flags, absolute library paths, unevaluated
      # generator expressions -- is not ours to attribute.
      if(_candidate MATCHES "^\\$<LINK_ONLY:(.+)>$")
        set(_candidate "${CMAKE_MATCH_1}")
      endif()
      if(_candidate MATCHES "[$<>/\\\\]" OR _candidate MATCHES "^-")
        continue()
      endif()
      if(NOT TARGET ${_candidate})
        continue()
      endif()
      # Imported targets come from the system or from find_package, so they are
      # not ours to install and CMake attributes them on its own.
      get_target_property(_imported "${_candidate}" IMPORTED)
      if(_imported)
        continue()
      endif()
      list(APPEND _queue "${_candidate}")
    endforeach()
  endwhile()

  set(${out_var} "${_seen}" PARENT_SCOPE)
endfunction()

_duetscreen_sbom_collect_deps(DuetScreen _duetscreen_sbom_targets)

set(_duetscreen_sbom_undeclared "")
foreach(_target IN LISTS _duetscreen_sbom_targets)
  if(NOT DEFINED _duetscreen_sbom_${_target}_VERSION)
    list(APPEND _duetscreen_sbom_undeclared "${_target}")
  endif()
endforeach()

if(_duetscreen_sbom_undeclared)
  string(REPLACE ";" ", " _duetscreen_sbom_undeclared_text
                 "${_duetscreen_sbom_undeclared}")
  message(
    FATAL_ERROR
      "DuetScreen links the following target(s) that the SBOM has no metadata "
      "for: ${_duetscreen_sbom_undeclared_text}. Add a "
      "_duetscreen_sbom_declare() entry for each of them in "
      "cmake/DuetScreenSbom.cmake so the generated SBOM does not misattribute "
      "their version, license and origin.")
endif()

# ----------------------------------------------------------------------------
# Install rules the SBOMs are derived from
# ----------------------------------------------------------------------------
# Only the executable is part of a normal `cmake --install`. The static
# archives are installed purely so install(SBOM) has an export set to describe,
# so they go into an EXCLUDE_FROM_ALL component that has to be asked for by
# name: `cmake --install <dir> --component sbom-deps`.
set(_duetscreen_sbom_dep_component sbom-deps)

function(_duetscreen_sbom_install_target target export_name)
  get_target_property(_type "${target}" TYPE)
  if(_type STREQUAL "EXECUTABLE")
    install(TARGETS ${target}
            EXPORT ${export_name}
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  else()
    install(TARGETS ${target}
            EXPORT ${export_name}
            COMPONENT ${_duetscreen_sbom_dep_component}
            EXCLUDE_FROM_ALL
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            # Several dependencies set PUBLIC_HEADER; without a destination
            # install(TARGETS) warns even though the component is never
            # installed by default.
            PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  endif()
endfunction()

# The project's own targets share one document named after the project, which
# lets install(SBOM) inherit the project metadata.
foreach(_target IN LISTS _duetscreen_sbom_targets)
  if(NOT _target IN_LIST _duetscreen_sbom_first_party)
    continue()
  endif()
  # CMake 4.4 does not yet render SPDX_LICENSE into the document -- only the
  # document-level LICENSE below comes out -- but set it so per-component
  # licenses appear for free once it does.
  set_target_properties(${_target} PROPERTIES
                        SPDX_LICENSE "${_duetscreen_sbom_${_target}_LICENSE}")
  _duetscreen_sbom_install_target("${_target}" DuetScreenArtifacts)
endforeach()

install(
  SBOM DuetScreen
  EXPORTS DuetScreenArtifacts
  FORMAT spdx
  VERSION "${DUETSCREEN_SBOM_VERSION}"
  LICENSE "${DUETSCREEN_SBOM_LICENSE}"
  DESCRIPTION "Duet3D touchscreen controller firmware"
  HOMEPAGE_URL "${DUETSCREEN_SBOM_HOMEPAGE}"
  DESTINATION "${DUETSCREEN_SBOM_DESTINATION}")

# Every third-party library gets its own export set and document so its real
# version, license and origin are recorded.
#
# Known gap in the experimental feature: the DuetScreen document refers to each
# dependency by the namespace of the export the dependency itself installs --
# `urn:lvgl:lvgl#Package`, `urn:fmt:fmt#Package` -- as a metadata-free stub,
# whereas install(SBOM) ids a package by plain target name, `urn:lvgl#Package`.
# So each dependency appears twice across the document set under two ids, with
# the metadata only on one of them, and nothing relates the two. Giving the
# export sets below the dependency's own namespace would reconcile the ids, but
# CMake rejects that: these targets are already in an export set of their own
# ("that is in multiple export sets ... An SBOM cannot attribute a dependency
# exported in more than one export set"). Correct-but-unlinked metadata beats
# linked stubs with no metadata, so this stands until the feature settles.
foreach(_target IN LISTS _duetscreen_sbom_targets)
  if(_target IN_LIST _duetscreen_sbom_first_party)
    continue()
  endif()

  set_target_properties(${_target} PROPERTIES
                        SPDX_LICENSE "${_duetscreen_sbom_${_target}_LICENSE}")
  _duetscreen_sbom_install_target("${_target}" "${_target}Artifacts")

  set(_description_arg "")
  if(_duetscreen_sbom_${_target}_DESCRIPTION)
    set(_description_arg DESCRIPTION
                         "${_duetscreen_sbom_${_target}_DESCRIPTION}")
  endif()

  install(
    SBOM ${_target}
    EXPORTS "${_target}Artifacts"
    NO_PROJECT_METADATA
    FORMAT spdx
    VERSION "${_duetscreen_sbom_${_target}_VERSION}"
    LICENSE "${_duetscreen_sbom_${_target}_LICENSE}"
    HOMEPAGE_URL "${_duetscreen_sbom_${_target}_HOMEPAGE}"
    ${_description_arg}
    DESTINATION "${DUETSCREEN_SBOM_DESTINATION}")
endforeach()

list(JOIN _duetscreen_sbom_targets " " _duetscreen_sbom_targets_text)
message(STATUS "SBOM generation enabled for DuetScreen "
               "${DUETSCREEN_SBOM_VERSION} (${DUETSCREEN_SBOM_LICENSE})")
message(STATUS "  covering: ${_duetscreen_sbom_targets_text}")
message(STATUS "  installed to: <prefix>/${DUETSCREEN_SBOM_DESTINATION}")
