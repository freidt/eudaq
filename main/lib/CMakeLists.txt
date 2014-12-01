option(USE_ROOT "Compiling main library using ROOT" OFF)
if (USE_ROOT)
   FIND_PACKAGE( ROOT REQUIRED )
   if (ROOT_FOUND)
     INCLUDE_DIRECTORIES( ${ROOT_INCLUDE_DIR} )
     LINK_DIRECTORIES( ${ROOT_LIBRARY_DIR} )
     ADD_DEFINITIONS(-DROOT_FOUND)
   endif (ROOT_FOUND)
endif (USE_ROOT)

option(USE_TINYXML "Compiling main library using TinyXML" OFF)
if (USE_TINYXML OR BUILD_palpidefs)
     FIND_PACKAGE( TINYXML REQUIRED )
     if (TINYXML_FOUND)
       INCLUDE_DIRECTORIES( ${TINYXML_INCLUDE_DIRS} )
       LINK_DIRECTORIES( ${TINYXML_LIBRARIES} )
       ADD_DEFINITIONS(-DUSE_TINYXML)
       message("Compiling main library using TinyXML")
     endif (TINYXML_FOUND)
endif (USE_TINYXML OR BUILD_palpidefs)

AUX_SOURCE_DIRECTORY( src library_sources )

ADD_LIBRARY( ${PROJECT_NAME} SHARED ${library_sources} )
SOURCE_GROUP(plugins .*Plugin.*cc)
SOURCE_GROUP(FileIO .*File.*cc)
SOURCE_GROUP(TCPIP .*Transport.*cc)
SOURCE_GROUP(TCPIP FILES src\\Serializable.cc src\\Producer.cc src\\CommandReceiver.cc src\\DataCollector.cc src\\DataSender.cc  src\\LogCollector.cc)
SOURCE_GROUP(Event .*Event.*cc)
SOURCE_GROUP(Utilities FILES src\\Utils.cc src\\Time.cc src\\Mutex.cc src\\EudaqThread.cc)

include_directories( ${INCLUDE_DIRECTORIES} )
link_directories( ${LINK_DIRECTORIES} )
target_link_libraries( ${PROJECT_NAME} ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES} ${TINYXML_LIBRARIES} ${ROOT_LIBRARIES} )


INSTALL(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
