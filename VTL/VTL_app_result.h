#ifndef _VTL_APP_RESULT_H
#define _VTL_APP_RESULT_H

#ifdef __cplusplus
extern "C"
{
#endif



typedef enum _VTL_AppResult 
{
    VTL_res_kOk = 0,
    // Ошибки файловой системы (чтение)
    VTL_res_video_fs_r_kMissingFileErr = 1,
    VTL_res_video_fs_r_kFileIsBusyErr,
    // Ошибки файловой системы (запись)
    VTL_res_video_fs_w_kFileIsBusyErr = 10,
    // Общие ошибки
    VTL_res_kUnsupportedFormat = 20, // Формат не поддерживается
    VTL_res_kParseError = 21,        // Ошибка парсинга
    VTL_res_kMemoryError = 22,       // Ошибка памяти
    VTL_res_kArgumentError = 23,     // Некорректный аргумент
    VTL_res_kIOError = 24,           // Ошибка ввода-вывода
    VTL_res_kUnknownError = 25,      // Неизвестная ошибка
    // Универсальные ошибки API субтитров
    VTL_res_kNullArgument = 100,     // NULL-аргумент
    VTL_res_kAllocError = 101,       // Ошибка выделения памяти
    VTL_res_kEndOfFile = 102,        // Конец файла
    VTL_res_kInvalidIndex = 103,     // Некорректный индекс
    VTL_res_kSubtitleTimeParseError = 110, // Ошибка парсинга времени субтитра
    VTL_res_kSubtitleFormatError = 111,    // Ошибка формата субтитра
    VTL_res_kSubtitleTextOverflow = 112,   // Переполнение текста субтитра
    VTL_res_kSubInit = 113,                // Ошибка инициализации субтитров
    // Ошибки прожига (burn)
    VTL_res_burn_kOpenInputFileError = 200,    // Ошибка открытия входного файла
    VTL_res_burn_kCreateOutputContextError = 201, // Ошибка создания выходного контекста
    VTL_res_burn_kCreateOutputStreamError = 202,  // Ошибка создания выходного потока
    VTL_res_burn_kInitFiltersError = 203,        // Ошибка инициализации фильтров
    VTL_res_burn_kSetupVideoEncoderError = 204,  // Ошибка настройки видеокодера
    VTL_res_burn_kSetupAudioEncoderError = 205,  // Ошибка настройки аудиокодера
    VTL_res_burn_kOpenOutputFileError = 206,     // Ошибка открытия выходного файла
    VTL_res_burn_kWriteHeaderError = 207,        // Ошибка записи заголовка
    VTL_res_burn_kAllocPacketOrFrameError = 208, // Ошибка выделения пакета/кадра
    VTL_res_burn_kWriteTrailerError = 209,       // Ошибка записи трейлера
    // Ошибки конвертации (convert)
    VTL_res_convert_kOpenInputFileError = 220,   // Ошибка открытия входного файла
    VTL_res_convert_kReadMetaError = 221,        // Ошибка чтения метаданных
    VTL_res_convert_kOpenOutputFileError = 222,  // Ошибка открытия выходного файла
    VTL_res_convert_kWritePartError = 223,       // Ошибка записи части файла
    VTL_res_convert_kAllocError = 224,           // Ошибка выделения памяти
    VTL_res_convert_kStyleApplyError = 225,      // Ошибка применения стиля
    // Ошибки стиля (style)
    VTL_res_style_kJsonParseError = 240,         // Ошибка парсинга JSON
    // Ошибки OpenCL
    VTL_res_opencl_kPlatformError = 260,         // Ошибка платформы OpenCL
    VTL_res_opencl_kDeviceError = 261,           // Ошибка устройства OpenCL
    VTL_res_opencl_kContextError = 262,          // Ошибка контекста OpenCL
    VTL_res_opencl_kQueueError = 263,            // Ошибка очереди OpenCL
    VTL_res_opencl_kProgramError = 264,          // Ошибка программы OpenCL
    VTL_res_opencl_kBuildError = 265,            // Ошибка сборки OpenCL
    VTL_res_opencl_kKernelError = 266,           // Ошибка ядра OpenCL
    VTL_res_opencl_kLaunchError = 267,           // Ошибка запуска ядра OpenCL
    VTL_res_opencl_kAllocError = 268,            // Ошибка выделения памяти OpenCL
    VTL_res_opencl_kReadBufferError = 269,       // Ошибка чтения буфера OpenCL
    VTL_res_opencl_kGeneralError = 270           // Общая ошибка OpenCL
} VTL_AppResult;




#ifdef __cplusplus
}
#endif


#endif