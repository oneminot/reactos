@ stdcall ODBCCPlApplet( long long ptr ptr)
@ stdcall SQLConfigDataSource(ptr long wstr wstr)
@ stdcall SQLConfigDataSourceW(ptr long str str)
@ stdcall SQLConfigDriver(ptr long str str ptr long ptr)
@ stdcall SQLConfigDriverW(ptr long wstr wstr ptr long ptr)
@ stdcall SQLCreateDataSource(ptr str)
@ stub SQLCreateDataSourceEx
@ stub SQLCreateDataSourceExW
@ stdcall SQLCreateDataSourceW(ptr wstr)
@ stdcall SQLGetAvailableDrivers(str ptr long ptr)
@ stdcall SQLGetAvailableDriversW(wstr ptr long ptr)
@ stdcall SQLGetConfigMode(ptr)
@ stdcall SQLGetInstalledDrivers(str long ptr)
@ stdcall SQLGetInstalledDriversW(wstr long ptr)
@ stdcall SQLGetPrivateProfileString(str str str str long str)
@ stdcall SQLGetPrivateProfileStringW(wstr wstr wstr wstr long wstr)
@ stdcall SQLGetTranslator(ptr str long ptr ptr long ptr ptr)
@ stdcall SQLGetTranslatorW(ptr wstr long ptr ptr long ptr ptr)
@ stdcall SQLInstallDriver(str str str long ptr)
@ stdcall SQLInstallDriverEx(str str str long ptr long ptr)
@ stdcall SQLInstallDriverExW(wstr wstr wstr long ptr long ptr)
@ stdcall SQLInstallDriverManager(ptr long ptr)
@ stdcall SQLInstallDriverManagerW(ptr long ptr)
@ stdcall SQLInstallDriverW(wstr wstr wstr long ptr)
@ stdcall SQLInstallODBC(ptr str str str)
@ stdcall SQLInstallODBCW(ptr wstr wstr wstr)
@ stdcall SQLInstallTranslator(str str str ptr long ptr long ptr)
@ stdcall SQLInstallTranslatorEx(str str ptr long ptr long ptr)
@ stdcall SQLInstallTranslatorExW(wstr wstr ptr long ptr long ptr)
@ stdcall SQLInstallTranslatorW(wstr wstr wstr ptr long ptr long ptr)
@ stdcall SQLInstallerError(long ptr ptr long ptr)
@ stdcall SQLInstallerErrorW(long ptr ptr long ptr)
@ stub SQLLoadDataSourcesListBox
@ stub SQLLoadDriverListBox
@ stdcall SQLManageDataSources(ptr)
@ stdcall SQLPostInstallerError(long ptr)
@ stdcall SQLPostInstallerErrorW(long ptr)
@ stdcall SQLReadFileDSN(str str str ptr long ptr)
@ stdcall SQLReadFileDSNW(wstr wstr wstr ptr long ptr)
@ stdcall SQLRemoveDSNFromIni(str)
@ stdcall SQLRemoveDSNFromIniW(wstr)
@ stdcall SQLRemoveDefaultDataSource()
@ stdcall SQLRemoveDriver(str long ptr)
@ stdcall SQLRemoveDriverManager(ptr)
@ stdcall SQLRemoveDriverW(wstr long ptr)
@ stdcall SQLRemoveTranslator(str ptr)
@ stdcall SQLRemoveTranslatorW(wstr ptr)
@ stdcall SQLSetConfigMode(long)
@ stdcall SQLValidDSN(str)
@ stdcall SQLValidDSNW(wstr)
@ stdcall SQLWriteDSNToIni(str str)
@ stdcall SQLWriteDSNToIniW(wstr wstr)
@ stdcall SQLWriteFileDSN(str str str str)
@ stdcall SQLWriteFileDSNW(wstr wstr wstr wstr)
@ stdcall SQLWritePrivateProfileString(str str str str)
@ stdcall SQLWritePrivateProfileStringW(wstr wstr wstr wstr)
@ stub SelectTransDlg
