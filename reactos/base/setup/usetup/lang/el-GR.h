#ifndef LANG_EL_GR_H__
#define LANG_EL_GR_H__

static MUI_ENTRY elGRLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Language Selection.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Please choose the language used for the installation process.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  This Language will be the default language for the final system.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue  F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "���� ��婘�� ���� �����ᩫ��� ��� ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "���� �� �⨦� ��� �����ᩫ���� ������᭜� �� ����������� �穫��� ReactOS ����",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "���������� ��� ��� ��������᝜� �� ��竜�� �⨦� ��� �����ᩫ����.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  ���㩫� ENTER ��� �� ��������㩜�� �� ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  ���㩫� R ��� �� ��������驜�� �� ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  ���㩫� L ��� �� ��嫜 ���� 樦�� ������櫞��� ��� ��� ���触�⩜�� ��� �� ReactOS",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  ���㩫� F3 ��� �� �����㩜�� ��� �� ��������㩜�� �� ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "��� ������櫜��� ��������圪 ��� �� ReactOS, ��������磜 ���������嫜 ��:",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.org",
        TEXT_HIGHLIGHT
    },
    {
        0,
        0,
        "   ENTER = ���⮜��  R = �����樟ਫ਼ F3 = ����騞��",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "� �����ᩫ��� ��� ReactOS �夘� �� ������ �ᩞ ���������������. ��� �������坜� ��棘",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "梜� ��� ����������櫞��� ���� ��㨦�� �����ᩫ���� ������������.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "�� ��棜��� ����������� ���禬�:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- � �����ᩫ��� �� ������ �� �������⢟�� �� ��� ��� ⤘ primary partition ��� �婡�.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- � �����ᩫ��� �� ������ �� �����ᯜ� ⤘ primary partition ��� ⤘ �婡�",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  ��橦� ��ᨮ��� extended partitions ��� �婡� ����.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- � �����ᩘ�� �� ������ �� �����ᯜ� �� ��髦 extended partition ��� �婡��",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  ��橦� ��ᨮ��� �� ᢢ� extended partitions ��� �婡� ����.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- � �����ᩫ��� �������坜� �椦 FAT ����㣘�� ������.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- File system checks are not implemented yet.",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  ���㩫� ENTER ��� �� ��������㩜�� �� ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  ���㩫� F3 ��� �� �����㩜�� ��� �� ��������㩜�� �� ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = ���⮜��   F3 = ����騞��",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        6,
        "Licensing:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        8,
        "The ReactOS System is licensed under the terms of the",
        TEXT_NORMAL
    },
    {
        8,
        9,
        "GNU GPL with parts containing code from other compatible",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "licenses such as the X11 or BSD and GNU LGPL licenses.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "All software that is part of the ReactOS system is",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "therefore released under the GNU GPL as well as maintaining",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "the original license.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "This software comes with NO WARRANTY or restrictions on usage",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "save applicable local and international law. The licensing of",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "ReactOS only covers distribution to third parties.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "If for some reason you did not receive a copy of the",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License with ReactOS please visit",
        TEXT_NORMAL
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_HIGHLIGHT
    },
    {
        8,
        22,
        "Warranty:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        24,
        "This is free software; see the source for copying conditions.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "There is NO warranty; not even for MERCHANTABILITY or",
        TEXT_NORMAL
    },
    {
        8,
        26,
        "FITNESS FOR A PARTICULAR PURPOSE",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Return",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "� ������� �婫� ��室�� ��� ����婜�� �������.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "       ����������:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "        ���ᤠ��:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "       ��������暠�:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "��᫘�� ����������妬:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "         �������:",
        TEXT_NORMAL
    },
    {
        25,
        16, "������� ���� �� ����婜� �������",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "�����嫜 �� ���ᥜ�� ��� ����婜�� ������ ���餫�� �� ��㡫�� UP � DOWN",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "��� �� ����⥜�� ��� �矣���. ���� ���㩫� �� ��㡫�� ENTER ��� �� ����⥜�� ᢢ��",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "����婜��.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "� 梜� �� ����婜�� �夘� �੫�, ����⥫� \"������� ���� �� ����婜� �������\"",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "��� ���㩫� ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = ���⮜��   F3 = ����騞��",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRRepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "� �����ᩫ��� ��� ReactOS ��婡���� �� ��頣� ��ᛠ� ��᧫����. ��� �������坜� ��棘",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "梜� ��� ����������櫞��� ���� ��㨦�� �����ᩫ���� ������������.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "� ��������圪 �����樟ਫ਼� ��� ⮦�� ���������� ��棘.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  ���㩫� U ��� �����ਫ਼ ��� OS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  ���㩫� R ��� �� Recovery Console.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  ���㩫� ESC ��� �� ������⯜�� ���� �稠� ���囘.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  ���㩫� ENTER ��� �� ���������㩜�� ��� ����������.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = �稠� ���囘  ENTER = �������夞��",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY elGRComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "�⢜�� �� ���ᥜ�� ��� �秦 ��� ���������� ��� �� ������������.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  ���㩫� �� ��㡫�� UP � DOWN ��� �� ����⥜�� ��� ��������� �秦 ����������.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   ���� ���㩫� ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  ���㩫� �� ��㡫�� ESC ��� �� ������⯜�� ���� ������磜�� ���囘 ��� �� ���ᥜ��",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   ��� �秦 ����������.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = ���⮜��   ESC = ���ਫ਼   F3 = ����騞��",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "�� �穫��� ��������餜� �騘 櫠 梘 �� �����⤘ �������竞��� ��� �婡�",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "���� �� �᨜� �嚞 騘",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "� �����������, � ���������� ��� �� ������������� ���棘��",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Flushing cache",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "�� ReactOS ��� ��������៞�� ����",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "�����⩫� �� ����⫘ ��� �� A: ���",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "梘 �� CD-ROMs  ��� �� CD-Drives.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "���㩫� ENTER ��� �� ���������㩜�� ��� ����������.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   �������� �����⤜�� ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "�⢜�� �� ���ᥜ�� ��� �秦 ��� ���ᤠ��� ��� �� ������������.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  ���㩫� �� ��㡫�� UP � DOWN ��� �� ����⥜�� ��� ��������� �秦 ���ᤠ���.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   ���� ���㩫� ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  ���㩫� �� ��㡫�� ESC ��� �� ������⯜�� ���� ������磜�� ���囘 ��� �� ���ᥜ��",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   ��� �秦 ���ᤠ���.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = ���⮜��   ESC = ���ਫ਼   F3 = ����騞��",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "�� ������ ������� ��� ReactOS ��������៞��� �������.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "�����⩫� �� ����⫘ ��� �� A: ���",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "梘 �� CD-ROMs ��� �� CD-Drive.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "���㩫� ENTER ��� �� ���������㩜�� ��� ���������� ���.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = �������夞�� ����������",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRBootPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "� �����ᩫ��� �� ������ �� ��������㩜� ��� bootloader ��� ������ �婡�",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "��� ���������� ���",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "�������� ���᚜�� ��� ��������⤞ ����⫘ ��� A: ���",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "���㩫� ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = ���⮜��   F3 = ����騞��",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY elGRSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "The list below shows existing partitions and unused disk",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "space for new partitions.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "\x07  Press UP or DOWN to select a list entry.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Press ENTER to install ReactOS onto the selected partition.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Press C to create a new partition.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Press D to delete an existing partition.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Please wait...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Format partition",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "Setup will now format the partition. Press ENTER to continue.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_NORMAL
    }
};

static MUI_ENTRY elGRInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Setup installs ReactOS files onto the selected partition. Choose a",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "directory where you want ReactOS to be installed:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "To change the suggested directory, press BACKSPACE to delete",
        TEXT_NORMAL
    },
    {
        6,
        15,
        "characters and then type the directory where you want ReactOS to",
        TEXT_NORMAL
    },
    {
        6,
        16,
        "be installed.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        11,
        12,
        "Please wait while ReactOS Setup copies files to your ReactOS",
        TEXT_NORMAL
    },
    {
        30,
        13,
        "installation folder.",
        TEXT_NORMAL
    },
    {
        20,
        14,
        "This may take several minutes to complete.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Please wait...    ",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRBootLoaderEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Setup is installing the boot loader",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Install bootloader on the harddisk (MBR).",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Install bootloader on a floppy disk.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Skip install bootloader.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "You want to change the type of keyboard to be installed.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard type.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   the keyboard type.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "You want to change the keyboard layout to be installed.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "    layout. Then press ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   the keyboard layout.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY elGRPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Setup prepares your computer for copying the ReactOS files. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Building the file copy list...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY elGRSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        17,
        "Select a file system from the list below.",
        0
    },
    {
        8,
        19,
        "\x07  Press UP or DOWN to select a file system.",
        0
    },
    {
        8,
        21,
        "\x07  Press ENTER to format the partition.",
        0
    },
    {
        8,
        23,
        "\x07  Press ESC to select another partition.",
        0
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "You have chosen to delete the partition",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  Press D to delete the partition.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "WARNING: All data on this partition will be lost!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Press ESC to cancel.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = Delete Partition   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRRegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Setup is updating the system configuration. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Creating registry hives...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR elGRErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "�� ReactOS ��� ��������៞�� ���� ����\n"
        "���������� ���. �� �����㩜�� ��� ��� �����ᩫ��� �騘, �� ��⧜� ��\n"
        "������⥜�� ��� �����ᩫ��� ��� �� ��������㩜� �� ReactOS.\n"
        "\n"
        "  \x07  ���㩫� ENTER ��� �� �����婜�� ��� �����ᩫ���.\n"
        "  \x07  ���㩫� F3 ��� �� �����㩜�� ��� ��� �����ᩫ���.",
        "F3= ����騞��  ENTER = ���⮜��"
    },
    {
        //ERROR_NO_HDD
        "� �����ᩫ��� �� ��樜�� �� ���� �᧦��� ������ �婡�.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup could not find its source drive.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "� �����ᩫ��� �� ��樜�� �� ����驜� �� ����� TXTSETUP.SIF.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "� �����ᩫ�� ��㡜 ⤘ ���������⤦ ����� TXTSETUP.SIF.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "� �����ᩫ��� ��㡜 ��� �� ⚡��� �������� ��� TXTSETUP.SIF.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "� �����ᩫ��� �� ��樜�� �� ����驜� ��� ��������圪 ��� �婡�� ����㣘���.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_WRITE_BOOT,
        "Setup failed to install FAT bootcode on the system partition.",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "� �����ᩫ��� �� ��樜�� �� ����驜� �� �婫� ��� ����������.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "� �����ᩫ��� �� ��樜�� �� ����驜� �� �婫� ��� ���ᤠ���.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "� �����ᩫ��� �� ��樜�� �� ����驜� �� �婫� ��� ����������妬.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "� �����ᩫ��� �� ��樜�� �� ����驜� �� �婫� ����ᥜ� ����������妬.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_WARN_PARTITION,
          "� �����ᩫ��� ��㡜 櫠 ����ᮠ���� ⤘� ������ �婡�� ����⮜� ⤘ �� �������\n"
          "partition table ��� �� ������ �� �������� �੫�!\n"
          "\n"
          "� ��������� � �������� partitions ������ �� �������⯜� �� partiton table.\n"
          "\n"
          "  \x07  ���㩫� F3 ��� �� �����㩜�� ��� ��� �����ᩫ���."
          "  \x07  ���㩫� ENTER ��� �� �����婜��.",
          "F3= ����騞��  ENTER = ���⮜��"
    },
    {
        //ERROR_NEW_PARTITION,
        "�� �����嫜 �� ��������㩜�� ⤘ Partition �⩘ ��\n"
        "⤘ ᢢ� ��ᨮ�� Partition!\n"
        "\n"
        "  * ���㩫� ������㧦�� ��㡫�� ��� �� �����婜��.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "�� �����嫜 �� �����ᯜ�� ⤘� �� ��������⤦ �騦 �婡��!\n"
        "\n"
        "  * ���㩫� ������㧦�� ��㡫�� ��� �� �����婜��.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup failed to install the FAT bootcode on the system partition.",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_NO_FLOPPY,
        "��� ��ᨮ�� ����⫘ ��� A:.",
        "ENTER = ���⮜��"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "� �����ᩘ�� ��⫬�� �� �����驜� ��� ����婜�� ��� �� ��᫘�� ����������妬.",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "� �����ᩫ��� ��⫬�� �� �����驜� ��� ����婜�� ����馬 ��� ��� ���ᤠ��.",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_IMPORT_HIVE,
        "� �����ᩫ��� ��⫬�� �� ����驜� ⤘ hive �����.",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_FIND_REGISTRY
        "� �����ᩘ�� ��⫬�� �� ���� �� ����� ������� ��� ����馬.",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_CREATE_HIVE,
        "� �����ᩫ��� ��⫬�� �� ��������㩜� �� registry hives.",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "�� cabinet ��� ⮜� ⚡��� ����� inf.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_CABINET_MISSING,
        "�� cabinet �� ��⟞��.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "�� cabinet ��� ⮜� ���⤘ ������ �����ᩫ����.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_COPY_QUEUE,
        "� �����ᩫ��� ��⫬�� �� ���奜� ��� ���� ������ ���� ���������.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_CREATE_DIR,
        "� �����ᩫ��� �� ��樜�� �� ��������㩜� ���� �����暦�� �����ᩫ����.",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "� �����ᩫ��� ��⫬�� �� ���� ��� ���� 'Directories'\n"
        "��� TXTSETUP.SIF.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_CABINET_SECTION,
        "� �����ᩫ��� ��⫬�� �� ���� ��� ���� 'Directories'\n"
        "��� cabinet.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "� �����ᩫ��� �� ��樜�� �� ��������㩜� ��� ���ᢦ�� �����ᩫ����.",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "� �����ᩫ��� ��⫬�� �� ���� ��� ���� 'SetupData'\n"
        "��� TXTSETUP.SIF.\n",
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_WRITE_PTABLE,
        "� �����ᩫ��� ��⫬�� �� ��ᯜ� �� partition tables.\n"
        "ENTER = �������夞�� ����������"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup failed to add codepage to registry.\n"
        "ENTER = Reboot computer"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup could not set the system locale.\n"
        "ENTER = Reboot computer"
    },
    {
        NULL,
        NULL
    }
};


MUI_PAGE elGRPages[] =
{
    {
        LANGUAGE_PAGE,
        elGRLanguagePageEntries
    },
    {
       START_PAGE,
       elGRWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        elGRIntroPageEntries
    },
    {
        LICENSE_PAGE,
        elGRLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        elGRDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        elGRRepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        elGRComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        elGRDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        elGRFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        elGRSelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        elGRSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        elGRFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        elGRDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        elGRInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        elGRPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        elGRFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        elGRKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        elGRBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        elGRLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        elGRQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        elGRSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        elGRBootPageEntries
    },
    {
        REGISTRY_PAGE,
        elGRRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING elGRStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Please wait..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   C = Create Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Install   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "Size of new partition:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a new partition on"},
    {STRING_HDDSIZE,
    "Please enter the size of the new partition in megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = Create Partition   ESC = Cancel   F3 = Quit"},
    {STRING_PARTFORMAT,
    "This Partition will be formatted next."},
    {STRING_NONFORMATTEDPART,
    "You chose to install ReactOS on a new or unformatted Partition."},
    {STRING_INSTALLONPART,
    "Setup install ReactOS onto Partition"},
    {STRING_CHECKINGPART,
    "Setup is now checking the selected partition."},
    {STRING_QUITCONTINUE,
    "F3= Quit  ENTER = Continue"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Reboot computer"},
    {STRING_TXTSETUPFAILED,
    "Setup failed to find the '%S' section\nin TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "                                                   \xB3 Copying file: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Setup is copying files..."},
    {STRING_PAGEDMEM,
     "Paged Memory"},
    {STRING_NONPAGEDMEM,
     "Nonpaged Memory"},
    {STRING_FREEMEM,
     "Free Memory"},
    {STRING_REGHIVEUPDATE,
    "   Updating registry hives..."},
    {STRING_IMPORTFILE,
    "   Importing %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Updating display registry settings..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Updating locale settings..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Updating keyboard layout settings..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Adding codepage information to registry..."},
    {STRING_DONE,
    "   Done..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Reboot computer"},
    {STRING_CONSOLEFAIL1,
    "Unable to open the console\n\n"},
    {STRING_CONSOLEFAIL2,
    "The most common cause of this is using an USB keyboard\n"},
    {STRING_CONSOLEFAIL3,
    "USB keyboards are not fully supported yet\n"},
    {STRING_FORMATTINGDISK,
    "Setup is formatting your disk"},
    {STRING_CHECKINGDISK,
    "Setup is checking your disk"},
    {STRING_FORMATDISK1,
    " Format partition as %S file system (quick format) "},
    {STRING_FORMATDISK2,
    " Format partition as %S file system "},
    {STRING_KEEPFORMAT,
    " Keep current file system (no changes) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Type %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK3,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Type %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "on Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c  Type %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "Setup created a new partition on"},
    {STRING_UNPSPACE,
    "    Unpartitioned space              %6lu %s"},
    {0, 0}
};

#endif
