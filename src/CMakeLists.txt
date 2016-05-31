# Add executable targets
########################
ADD_EXECUTABLE(light_controller_appd light_controller.c)
# Add library targets
#####################
FIND_LIBRARY(LIB_FLOWCORE libflowcore.so PATHS ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_FLOWMESSAGING libflowmessaging.so PATHS ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_FLOWDEVMGMT libawa.so PATHS ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_CONFIG libconfig.so PATHS ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_GNUTLS libgnutls.so PATHS ${STAGING_DIR}/usr/lib)
TARGET_LINK_LIBRARIES(light_controller_appd ${LIB_FLOWDEVMGMT} ${LIB_GNUTLS} ${LIB_FLOWCORE} ${LIB_FLOWMESSAGING} ${LIB_CONFIG})

# Add install targets
######################
INSTALL(TARGETS light_controller_appd RUNTIME DESTINATION bin)
