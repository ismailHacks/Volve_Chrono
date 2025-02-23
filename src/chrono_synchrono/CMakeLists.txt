#=============================================================================
# CMake configuration file for SynChrono module
#
# Cannot be used stand-alone (it is loaded by parent CMake configuration file)
#=============================================================================

option(ENABLE_MODULE_SYNCHRONO "Enable the SynChrono module" OFF)

IF(NOT ENABLE_MODULE_SYNCHRONO)
  return()
ENDIF()

message(STATUS "==== SynChrono module ====")

# Return now if MPI is not available
if(NOT MPI_CXX_FOUND)
	message(WARNING "Chrono::SynChrono requires MPI, but MPI not found; disabling Chrono::SynChrono")
	set(ENABLE_MODULE_SYNCHRONO OFF CACHE BOOL "Enable the SynChrono module" FORCE)
	return()
endif()

# Return now if Chrono::Vehicle is not enabled
if(NOT ENABLE_MODULE_VEHICLE)
    message(WARNING "Chrono::SynChrono depends on Chrono::Vehicle which is disabled; disabling Chrono::SynChrono")
    set(ENABLE_MODULE_SYNCHRONO OFF CACHE BOOL "Enable the SynChrono module" FORCE)
    return()
endif()

set(SYN_INCLUDES "${CMAKE_SOURCE_DIR}/src/chrono_thirdparty/flatbuffers/include")

set(SYN_CXX_FLAGS "${CH_CXX_FLAGS}")
set(SYN_LINKER_FLAGS "${CH_LINKERFLAG_SHARED}")
set(SYN_LIBRARIES "")
set(SYN_LIB_NAMES "ChronoEngine")

#-----------------------------------------------------------------------------
# Find MPI (Default transport method)
#-----------------------------------------------------------------------------

set(SYN_INCLUDES
    ${SYN_INCLUDES}
    ${MPI_C_HEADER_DIR}
    ${MPI_CXX_HEADER_DIR}
)

set(SYN_LIBRARIES
    ${SYN_LIBRARIES}
    ${MPI_LIBRARIES}
)

# Generate the configuration header file using substitution variables.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/SynConfig.h.in
			   ${PROJECT_BINARY_DIR}/chrono_synchrono/SynConfig.h)

install(FILES "${PROJECT_BINARY_DIR}/chrono_synchrono/SynConfig.h"
		DESTINATION include/chrono_synchrono)

# ------------
# Common Files
# ------------

set(SYN_BASE_FILES
	SynApi.h

	SynChronoManager.h
	SynChronoManager.cpp
)
source_group("base" FILES ${SYN_BASE_FILES})

set(SYN_AGENT_FILES
	agent/SynAgent.h
	agent/SynAgent.cpp

	agent/SynAgentFactory.h
	agent/SynAgentFactory.cpp

	agent/SynWheeledVehicleAgent.h
	agent/SynWheeledVehicleAgent.cpp
	agent/SynTrackedVehicleAgent.h
	agent/SynTrackedVehicleAgent.cpp

	agent/SynSCMTerrainAgent.h
	agent/SynSCMTerrainAgent.cpp
    
    agent/SynEnvironmentAgent.h
    agent/SynEnvironmentAgent.cpp
)
source_group("agent" FILES ${SYN_AGENT_FILES})

set(SYN_CONTROLLER_FILES
	controller/SynControllerFunctions.h
	controller/SynControllerFunctions.cpp
	
    controller/driver/SynMultiPathDriver.h
    controller/driver/SynMultiPathDriver.cpp
)
source_group("controller" FILES ${SYN_CONTROLLER_FILES})

set(SYN_COMMUNICATION_FILES
	communication/SynCommunicator.h
	communication/SynCommunicator.cpp
    
    communication/mpi/SynMPICommunicator.h
    communication/mpi/SynMPICommunicator.cpp
)
source_group("communication" FILES ${SYN_COMMUNICATION_FILES})

set(SYN_FLATBUFFER_FILES
	flatbuffer/SynFlatBuffersManager.h
	flatbuffer/SynFlatBuffersManager.cpp
	
	flatbuffer/message/SynFlatBuffers_generated.h

    flatbuffer/message/SynMessage.h
    flatbuffer/message/SynSimulationMessage.h
    flatbuffer/message/SynSimulationMessage.cpp

	flatbuffer/message/SynWheeledVehicleMessage.h
	flatbuffer/message/SynWheeledVehicleMessage.cpp
	flatbuffer/message/SynTrackedVehicleMessage.h
	flatbuffer/message/SynTrackedVehicleMessage.cpp
	
	flatbuffer/message/SynSCMMessage.h
	flatbuffer/message/SynSCMMessage.cpp
    
    flatbuffer/message/SynApproachMessage.h
    flatbuffer/message/SynApproachMessage.cpp
    flatbuffer/message/SynEnvironmentMessage.h
    flatbuffer/message/SynEnvironmentMessage.cpp
	flatbuffer/message/SynMAPMessage.h
	flatbuffer/message/SynMAPMessage.cpp
    flatbuffer/message/SynSPATMessage.h
	flatbuffer/message/SynSPATMessage.cpp

    flatbuffer/message/SynMessageUtils.h
    flatbuffer/message/SynMessageUtils.cpp
    flatbuffer/message/SynMessageFactory.h
    flatbuffer/message/SynMessageFactory.cpp
)
source_group("flatbuffer" FILES ${SYN_FLATBUFFER_FILES})

set(SYN_UTILS_FILES
	utils/SynDataLoader.h
	utils/SynDataLoader.cpp
    utils/SynGPSTools.h
    utils/SynGPSTools.cpp
    utils/SynLog.h
    utils/SynLog.cpp
)
source_group("utils" FILES ${SYN_UTILS_FILES})

#-----------------------------------------------------------------------------
# Create the ChronoEngine_synchrono library
#-----------------------------------------------------------------------------

if (ENABLE_MODULE_SENSOR)
  list(APPEND SYN_LIB_NAMES ChronoEngine_sensor)
endif()
  
if (ENABLE_MODULE_IRRLICHT)
  list(APPEND SYN_LIB_NAMES ChronoEngine_irrlicht)
  list(APPEND SYN_LIB_NAMES ChronoEngine_vehicle_irrlicht)

  set(SYN_INCLUDES  ${SYN_INCLUDES} ${CH_IRRLICHTINC})
  set(SYN_CXX_FLAGS "${SYN_CXX_FLAGS} ${CH_IRRLICHT_CXX_FLAGS}")
endif()

# Chrono::Vehicle is required
list(APPEND SYN_LIB_NAMES ChronoEngine_vehicle)
list(APPEND SYN_LIB_NAMES ChronoModels_vehicle)

# Set some variables to be visible outside this directory
SET(SYN_INCLUDES     "${SYN_INCLUDES}"  PARENT_SCOPE)
SET(SYN_LIBRARIES    "${SYN_LIBRARIES}" PARENT_SCOPE)
SET(SYN_CXX_FLAGS    "${SYN_CXX_FLAGS}" PARENT_SCOPE)

include_directories(${SYN_INCLUDES})

add_library(ChronoEngine_synchrono SHARED
	${SYN_BASE_FILES}
	${SYN_CONTROLLER_FILES}
    ${SYN_AGENT_FILES}
    ${SYN_COMMUNICATION_FILES}
    ${SYN_FLATBUFFER_FILES}
    ${SYN_UTILS_FILES}
)

# windows builds should disable warning 4661 and 4005
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4661 /wd4005")
endif()

set_target_properties(ChronoEngine_synchrono PROPERTIES
                      COMPILE_FLAGS "${SYN_CXX_FLAGS}"
                      LINK_FLAGS "${SYN_LINKER_FLAGS}")

target_compile_definitions(ChronoEngine_synchrono PRIVATE "SYN_API_COMPILE")

target_include_directories(ChronoEngine_synchrono PUBLIC ${SYN_INCLUDES})

target_link_libraries(ChronoEngine_synchrono ${SYN_LIB_NAMES} ${SYN_LIBRARIES})

install(TARGETS ChronoEngine_synchrono
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

#-------------------------------------------------------------------------------
# Install SynChrono headers
#-------------------------------------------------------------------------------

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION include/chrono_synchrono
        FILES_MATCHING PATTERN "*.h" PATTERN "fbs" EXCLUDE)

# Install required chrono_thirdparty headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/chrono_thirdparty/flatbuffers/include
        DESTINATION include/chrono_thirdparty/flatbuffers
        FILES_MATCHING PATTERN "*.h" PATTERN "*.cuh" PATTERN "*.hpp" PATTERN "*.inl")
