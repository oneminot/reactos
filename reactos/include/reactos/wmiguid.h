
#pragma once

#define IOCTL_WMI_QUERY_ALL_DATA CTL_CODE(FILE_DEVICE_UNKNOWN, 0x00, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224000
#define IOCTL_WMI_SINGLE_INSTANCE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x01, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224004
#define IOCTL_WMI_SET_SINGLE_INSTANCE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x02, METHOD_BUFFERED, FILE_WRITE_ACCESS) // 0x228008
#define IOCTL_WMI_SET_SINGLE_ITEM CTL_CODE(FILE_DEVICE_UNKNOWN, 0x03, METHOD_BUFFERED, FILE_WRITE_ACCESS) // 0x22800C
#define IOCTL_WMI_09 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x09, METHOD_BUFFERED, FILE_WRITE_ACCESS) // 0x228024
#define IOCTL_WMI_20 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x20, METHOD_BUFFERED, FILE_ANY_ACCESS) // 0x220080
#define IOCTL_WMI_21 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x21, METHOD_BUFFERED, FILE_ANY_ACCESS) // 0x220084
#define IOCTL_WMI_22 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x22, METHOD_BUFFERED, FILE_ANY_ACCESS) // 0x220088
#define IOCTL_WMI_TRACE_EVENT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x23, METHOD_NEITHER, FILE_WRITE_ACCESS) // 0x22808F
#define IOCTL_WMI_24 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x24, METHOD_BUFFERED, FILE_ANY_ACCESS) // 0x220090
#define IOCTL_WMI_25 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x25, METHOD_BUFFERED, FILE_ANY_ACCESS) // 0x220094
#define IOCTL_WMI_TRACE_USER_MESSAGE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x28, METHOD_NEITHER, FILE_WRITE_ACCESS) // 0x2280A3
#define IOCTL_WMI_29 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x29, METHOD_BUFFERED, FILE_ANY_ACCESS) // 0x2200A4
#define IOCTL_WMI_2a CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2a, METHOD_BUFFERED, FILE_ANY_ACCESS) // 0x2200A8
#define IOCTL_WMI_2b CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2b, METHOD_BUFFERED, FILE_ANY_ACCESS) // 0x2200AC
#define IOCTL_WMI_42 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x42, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224108
#define IOCTL_WMI_47 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x47, METHOD_BUFFERED, FILE_WRITE_ACCESS) // 0x22811C
#define IOCTL_WMI_49 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x49, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224124
#define IOCTL_WMI_4b CTL_CODE(FILE_DEVICE_UNKNOWN, 0x4b, METHOD_BUFFERED, FILE_WRITE_ACCESS) // 0x22812C
#define IOCTL_WMI_4c CTL_CODE(FILE_DEVICE_UNKNOWN, 0x4c, METHOD_BUFFERED, FILE_WRITE_ACCESS) // 0x228130
#define IOCTL_WMI_4d CTL_CODE(FILE_DEVICE_UNKNOWN, 0x4d, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224134
#define IOCTL_WMI_4e CTL_CODE(FILE_DEVICE_UNKNOWN, 0x4e, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224138
#define IOCTL_WMI_4f CTL_CODE(FILE_DEVICE_UNKNOWN, 0x4f, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x22413C
#define IOCTL_WMI_OPEN_GUID_FOR_EVENTS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x50, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224140
#define IOCTL_WMI_51 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x51, METHOD_BUFFERED, FILE_WRITE_ACCESS) // 0x228144
#define IOCTL_WMI_52 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x52, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224148
#define IOCTL_WMI_REGISTER_GUIDS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x53, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x22414C, called from ntdll!EtwpRegisterGuids
#define IOCTL_WMI_54 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x54, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224150
#define IOCTL_WMI_55 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x55, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224154
#define IOCTL_WMI_56 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x56, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224158
#define IOCTL_WMI_57 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x57, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x22415C
#define IOCTL_WMI_58 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x58, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224160
#define IOCTL_WMI_59 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x59, METHOD_BUFFERED, FILE_READ_ACCESS) // 0x224164
#define IOCTL_WMI_5a CTL_CODE(FILE_DEVICE_UNKNOWN, 0x5a, METHOD_BUFFERED, FILE_WRITE_ACCESS) // 0x228168
