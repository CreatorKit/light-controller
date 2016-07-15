# Add executable targets
########################
ADD_EXECUTABLE(motion_led_controller_appd motion_led_controller.c)

# Add library targets
#####################
FIND_LIBRARY(LIB_AWA libawa.so PATHS ${STAGING_DIR}/usr/lib)
TARGET_LINK_LIBRARIES(motion_led_controller_appd ${LIB_AWA})

# Add install targets
######################
INSTALL(TARGETS motion_led_controller_appd RUNTIME DESTINATION bin)
