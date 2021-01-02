//
// internet.h
// 

void ReadHTTPData_VersionInfo(void);
OSStatus LaunchURL(ConstStr255Param urlStr);
Boolean IsInternetAvailable(void);
OSStatus DownloadURL(const char *urlString);

