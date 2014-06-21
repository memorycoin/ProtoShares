
file(GLOB MEMORYCOIN_TS_FILES *.ts)

# Generate intermediate Qt-specific files from .ui and .h
qt4_add_translation(MEMORYCOIN_QM_FILES ${MEMORYCOIN_TS_FILES})
add_custom_target(memorycoin_translations DEPENDS ${MEMORYCOIN_QM_FILES})
set(MEMORYCOIN_QT_TRANSLATIONS ${QT_TRANSLATIONS})


