# Add executable targets
########################
ADD_EXECUTABLE(light_controller_appd light_controller.c)

# Add library targets
#####################
FIND_LIBRARY(LIB_AWA libawa.so PATHS ${STAGING_DIR}/usr/lib)
TARGET_LINK_LIBRARIES(light_controller_appd ${LIB_AWA})

# Add install targets
######################
INSTALL(TARGETS light_controller_appd RUNTIME DESTINATION bin)
