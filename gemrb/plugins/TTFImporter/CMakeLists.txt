IF (FREETYPE_LIBRARY)
	INCLUDE_DIRECTORIES( ${FREETYPE_INCLUDE_DIRS})
	INCLUDE_DIRECTORIES(${Iconv_INCLUDE_DIR})

	ADD_GEMRB_PLUGIN (TTFImporter TTFFontManager.cpp TTFFont.cpp Freetype.cpp)
	TARGET_LINK_LIBRARIES( TTFImporter ${FREETYPE_LIBRARY} )
	TARGET_LINK_LIBRARIES(TTFImporter ${Iconv_LIBRARY} ${CHARSET_LIBRARY})
ENDIF (FREETYPE_LIBRARY)
