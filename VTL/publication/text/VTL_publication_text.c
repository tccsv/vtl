#include <VTL/publication/text/VTL_publication_text.h>


// // static VTL_AppResult VTL_publication_marked_text_Init(VTL_publication_MarkedText **pp_marked_text, 
// //                                                 const VTL_publication_Text *p_src_text, 
// //                                                 const VTL_publication_marked_text_MarkupType src_markup_type)
// // {
// //     if(src_markup_type == VTL_markup_type_kStandartMD)
// //     {        
// //         VTL_publication_text_InitFromStandartMD(pp_marked_text, p_src_text);
// //     }
// //     else if(src_markup_type == VTL_markup_type_kTelegramMD)
// //     {
// //         VTL_publication_text_InitFromTelegramMD(pp_marked_text, p_src_text);
// //     }
// //     else if(src_markup_type == VTL_markup_type_kHTML)
// //     {
// //         VTL_publication_text_InitFromHTML(pp_marked_text, p_src_text);
// //     }
// //     else if(src_markup_type == VTL_markup_type_kBB)
// //     {
// //         VTL_publication_text_InitFromBB(pp_marked_text, p_src_text);
// //     }
// // }




// // VTL_AppResult VTL_publication_marked_text_TransformToStandartMD(VTL_publication_Text** pp_out_marked_text,
// //                                                     const VTL_publication_MarkedText* p_src_marked_text)
// // {
// //     return VTL_res_kOk;
// // }

// // VTL_AppResult VTL_publication_marked_text_TransformToTelegramMD(VTL_publication_Text** pp_out_marked_text,
// //                                                     const VTL_publication_MarkedText* p_src_marked_text)
// // {
// //     return VTL_res_kOk;
// // }

// // VTL_AppResult VTL_publication_marked_text_TransformToHTML(VTL_publication_Text** pp_out_marked_text,
// //                                                     const VTL_publication_MarkedText* p_src_marked_text)
// // {
// //     return VTL_res_kOk;
// // }

// // VTL_AppResult VTL_publication_marked_text_TransformToBB(VTL_publication_Text** pp_out_marked_text,
// //                                                     const VTL_publication_MarkedText* p_src_marked_text)
// // {
// //     return VTL_res_kOk;
// // }

// static VTL_AppResult VTL_publication_marked_text_GenStandartMDFile(const VTL_publication_MarkedText* p_src_marked_text, 
//                                                             const VTL_Filename out_file_name)
// {
//     VTL_publication_Text* p_out_text;
//     VTL_publication_marked_text_TransformToStandartMD(&p_out_text, p_src_marked_text);
//     VTL_pusblication_text_Write(p_out_text, out_file_name);
//     return VTL_res_kOk;
// }

// static VTL_AppResult VTL_publication_marked_text_CheckAndGenStandartMDFile(VTL_publication_MarkedText* p_src_marked_text,
//                                                     const VTL_Filename src_file_name, 
//                                                     const VTL_publication_marked_text_MarkupType src_markup_type,
//                                                     const VTL_Filename out_file_name,
//                                                     const VTL_publication_marked_text_type_Flags flags)
// {
//     if( VTL_publication_marked_text_type_flag_CheckStandartMD(flags) )
//     {
//         if( VTL_publication_marked_text_type_flag_CheckStandartMD(src_markup_type) )
//         {
//             if(!VTL_file_CheckEquality(src_file_name, out_file_name) )
//             {
//                 VTL_file_Copy(out_file_name, src_file_name);
//             }
//         }
//         else
//         {
//             VTL_publication_marked_text_GenStandartMDFile(p_src_marked_text, out_file_name);
//         }
//     }
// }

// static void VTL_publication_marked_text_CheckAndGen(const VTL_publication_MarkedText* p_marked_text,
//                                                     const VTL_Filename src_file_name, 
//                                                     const VTL_publication_marked_text_MarkupType src_markup_type,
//                                                     const VTL_publication_marked_text_Configs* p_configs)
// {

// }

// VTL_AppResult VTL_publication_marked_text_GenFiles(const VTL_Filename src_file_name, 
//                                                     const VTL_publication_marked_text_MarkupType src_markup_type,
//                                                     const VTL_publication_marked_text_Configs* p_configs)
// {
//     VTL_publication_MarkedText* p_marked_text;
//     VTL_publication_Text* p_src_text;
//     VTL_pusblication_text_Read(&p_src_text, src_file_name);
//     VTL_publication_marked_text_Init(&p_marked_text, p_src_text, src_markup_type, src_markup_type);
//     VTL_publication_marked_text_CheckAndGenStandartMDFile(p_marked_text, src_file_name, src_markup_type, 
//                                                             p_configs->files[VTL_markup_type_kStandartMD], p_configs->flags);
//     // if( VTL_publication_marked_text_type_flag_CheckStandartMD(p_configs->flags) )
//     // {
//     //     if( VTL_publication_marked_text_type_flag_CheckStandartMD(src_markup_type) )
//     //     {
//     //         if(!VTL_file_CheckEquality(src_file_name, p_configs->files[VTL_markup_type_kStandartMD]) )
//     //         {
//     //             VTL_file_Copy(p_configs->files[VTL_markup_type_kStandartMD], src_file_name);
//     //         }
//     //     }
//     //     else
//     //     {
//     //         VTL_publication_marked_text_GenStandartMDFile(p_marked_text, p_configs->files[VTL_markup_type_kStandartMD]);
//     //     }
//     // }
//     // if()
//     // if()
//     return VTL_res_kOk;
// }

