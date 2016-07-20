# Project Properties
####################
CMAKE_MINIMUM_REQUIRED (VERSION 2.6.2)
PROJECT(motion_led_controller)
# This will be extended to take the version from SCM
# Version config can be simplified from 3.0+
SET(motion_led_controller_VERSION_MAJOR 0)
SET(motion_led_controller_VERSION_MINOR 9)
SET(motion_led_controller_VERSION_PATCH 1)
SET(motion_led_controller_VERSION
    ${motion_led_controller_VERSION_MAJOR}.${motion_led_controller_VERSION_MINOR}.${motion_led_controller_VERSION_PATCH})

# Options
###################
SET(CMAKE_VERBOSE_MAKEFILE 1)
SET(CMAKE_BUILD_TYPE DEBUG) # Options MINSIZEREL, RELEASE, DEBUG

# Paths
########
ADD_SUBDIRECTORY(src)
