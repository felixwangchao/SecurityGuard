#include "pe_dialog.h"
#include "ui_pe_dialog.h"
#include <windows.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QMovie>

//
//  TypeOffset结构体的定义
//

typedef struct TypeOffset {
    WORD Offset : 12;  // (1) 大小为12Bit的重定位偏移
    WORD Type : 4;    // (2) 大小为4Bit的重定位信息类型值
}TypeOffset;	    // 这个结构体是A1Pass总结的


//
// RVA转文件偏移
//

DWORD RVAToOffset(LPVOID pDos, DWORD dwRva  )
{
    // 找到需要转换的RVA所在的区段,然后计算偏移.
    IMAGE_DOS_HEADER* pDosHdr = (IMAGE_DOS_HEADER*)pDos;

    IMAGE_NT_HEADERS* pNtHdr =
        (IMAGE_NT_HEADERS*)( (DWORD)pDos + pDosHdr->e_lfanew );

    IMAGE_SECTION_HEADER* pScnHdr =IMAGE_FIRST_SECTION( pNtHdr );

    DWORD dwScnCount = pNtHdr->FileHeader.NumberOfSections;

    for( DWORD i = 0; i < dwScnCount; i++ ) {

        //char* name = (char*)(pScnHdr[i].Name);
        //qDebug() << name;

        // 判断RVA是否在一个区段内
        if( dwRva >= pScnHdr[ i ].VirtualAddress
            && dwRva < pScnHdr[ i ].VirtualAddress + pScnHdr[ i ].SizeOfRawData ) {

            return dwRva - pScnHdr[ i ].VirtualAddress + pScnHdr[ i ].PointerToRawData;
        }
    }
    return -1;
}


PE_Dialog::PE_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PE_Dialog)
{
    ui->setupUi(this);

    if(ui->tree_importTable->headerItem())
    {
        ui->tree_importTable->headerItem()->setText(0, "");
    }

    if(ui->tree_delayLoad->headerItem())
    {
        ui->tree_delayLoad->headerItem()->setText(0, "");
    }

    QMovie *movie = new QMovie("./bear.gif");
    ui->label->setMovie(movie);
    ui->label->show();
    movie->start();
}

PE_Dialog::~PE_Dialog()
{
    delete ui;
}

void PE_Dialog::on_button_openFile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("open file")," ",tr("Allfile(*.*)"));

    if(fileName == NULL)
    {
        return;
    }
    CONST WCHAR* FileName = reinterpret_cast<CONST WCHAR *>(fileName.utf16());
    HANDLE hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        QMessageBox::information(NULL,"Info","文件无法打开\n",QMessageBox::Yes);
    }
    else
    {
        ui->table_dataDirectory->clearContents();
        ui->table_exportTable->clearContents();
        ui->table_fileHeader->clearContents();
        ui->tree_importTable->clear();
        ui->table_optHeader->clearContents();
        ui->table_sectionTable->clearContents();
        ui->tree_resource->clear();
        ui->tree_reloc->clear();
        ui->line_filename->setText(fileName);
        ParsePE(hFile);
    }
}

int PE_Dialog::ParsePE(HANDLE hFile)
{
    DWORD dwFileSize = GetFileSize(hFile,
            NULL);

    BYTE *pFileBuff = new BYTE[dwFileSize];

    // 将文件读取到内存中
    DWORD dwRead = 0;
    // 读取一个DOS头
    ReadFile(hFile,
        pFileBuff,
        dwFileSize,
        &dwRead,
        NULL
        );
    IMAGE_DOS_HEADER* pDosHdr = nullptr;
    pDosHdr = (IMAGE_DOS_HEADER*)pFileBuff;

    // 判断是否是有效的PE文件
    if (pDosHdr->e_magic != IMAGE_DOS_SIGNATURE) {
        QMessageBox::information(NULL,"Info","不是一个有效的PE文件\n",QMessageBox::Yes);
        system("pause");
        exit(0);
    }

    // 找到NT头
    IMAGE_NT_HEADERS* pNtHdr = nullptr;
    pNtHdr = (IMAGE_NT_HEADERS*)
        (pDosHdr->e_lfanew + (DWORD)pFileBuff);

    // 判断是否是有效的PE文件
    if (pNtHdr->Signature != IMAGE_NT_SIGNATURE) {
        QMessageBox::information(NULL,"Info","不是一个有效的PE文件\n",QMessageBox::Yes);
        system("pause");
        exit(0);
    }

    // 得到文件头
    IMAGE_FILE_HEADER* pFileHdr = nullptr;
    pFileHdr = &pNtHdr->FileHeader;

    QString machine;
    switch(pFileHdr->Machine)
    {
    case 0x14c:
        machine = QString("Inteli386");
        break;
    case 0x162:
        machine = QString("MIPS R3000");
        break;
    case 0x166:
        machine = QString("MIPS R4000");
        break;
    case 0x184:
        machine = QString("Alpha AXP");
        break;
    case 0x1F0:
        machine = QString("Power PC");
        break;
    }

    ui->table_fileHeader->setRowCount(7);
    ui->table_fileHeader->setColumnCount(2);

    QStringList header;
    header<< "Type"  << "Value";
    ui->table_fileHeader->setHorizontalHeaderLabels(header);

    ui->table_fileHeader->setItem(0,0,new QTableWidgetItem(QString("Machine")));
    ui->table_fileHeader->setItem(1,0,new QTableWidgetItem(QString("NumberOfSections")));
    ui->table_fileHeader->setItem(2,0,new QTableWidgetItem(QString("TimeDataStamp")));
    ui->table_fileHeader->setItem(3,0,new QTableWidgetItem(QString("PointerToSymbolTable")));
    ui->table_fileHeader->setItem(4,0,new QTableWidgetItem(QString("NumberOfSymbols")));
    ui->table_fileHeader->setItem(5,0,new QTableWidgetItem(QString("SizeOfOptionalHeader")));
    ui->table_fileHeader->setItem(6,0,new QTableWidgetItem(QString("Characteristics")));

    ui->table_fileHeader->setItem(0,1,new QTableWidgetItem(machine));
    ui->table_fileHeader->setItem(1,1,new QTableWidgetItem(QString("0x%1").arg(pFileHdr->NumberOfSections,0,16)));
    ui->table_fileHeader->setItem(2,1,new QTableWidgetItem(QString("0x%1").arg(pFileHdr->TimeDateStamp,0,16)));
    ui->table_fileHeader->setItem(3,1,new QTableWidgetItem(QString("0x%1").arg(pFileHdr->PointerToSymbolTable,0,16)));
    ui->table_fileHeader->setItem(4,1,new QTableWidgetItem(QString("0x%1").arg(pFileHdr->NumberOfSymbols,0,16)));
    ui->table_fileHeader->setItem(5,1,new QTableWidgetItem(QString("0x%1").arg(pFileHdr->SizeOfOptionalHeader,0,16)));
    ui->table_fileHeader->setItem(6,1,new QTableWidgetItem(QString("0x%1").arg(pFileHdr->Characteristics,0,16)));
    ui->table_fileHeader->show();

    ui->table_fileHeader->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_fileHeader->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    IMAGE_OPTIONAL_HEADER* pOptHdr = &pNtHdr->OptionalHeader;

    ui->table_optHeader->setRowCount(29);
    ui->table_optHeader->setColumnCount(2);
    ui->table_optHeader->setHorizontalHeaderLabels(header);
    ui->table_optHeader->setItem(0,0,new QTableWidgetItem(QString("Magic")));
    ui->table_optHeader->setItem(1,0,new QTableWidgetItem(QString("MajorLinkerVersion")));
    ui->table_optHeader->setItem(2,0,new QTableWidgetItem(QString("MinorLinkerVersion")));
    ui->table_optHeader->setItem(3,0,new QTableWidgetItem(QString("SizeOfCode")));
    ui->table_optHeader->setItem(4,0,new QTableWidgetItem(QString("SizeOfInitializedData")));
    ui->table_optHeader->setItem(5,0,new QTableWidgetItem(QString("SizeOfUninitializedData")));
    ui->table_optHeader->setItem(6,0,new QTableWidgetItem(QString("AddressOfEntryPoint")));
    ui->table_optHeader->setItem(7,0,new QTableWidgetItem(QString("BaseOfCode")));
    ui->table_optHeader->setItem(8,0,new QTableWidgetItem(QString("BaseOfData")));
    ui->table_optHeader->setItem(9,0,new QTableWidgetItem(QString("ImageBase")));
    ui->table_optHeader->setItem(10,0,new QTableWidgetItem(QString("SectionAlignment")));
    ui->table_optHeader->setItem(11,0,new QTableWidgetItem(QString("FileAlignment")));
    ui->table_optHeader->setItem(12,0,new QTableWidgetItem(QString("MajorOperatingSystemVersion")));
    ui->table_optHeader->setItem(13,0,new QTableWidgetItem(QString("MinorOperatingSystemVersion")));
    ui->table_optHeader->setItem(14,0,new QTableWidgetItem(QString("MajorImageVersion")));
    ui->table_optHeader->setItem(15,0,new QTableWidgetItem(QString("MinorImageVersion")));
    ui->table_optHeader->setItem(16,0,new QTableWidgetItem(QString("Win32VersionValue")));
    ui->table_optHeader->setItem(17,0,new QTableWidgetItem(QString("SizeOfImage")));
    ui->table_optHeader->setItem(18,0,new QTableWidgetItem(QString("SizeOfHeaders")));
    ui->table_optHeader->setItem(19,0,new QTableWidgetItem(QString("CheckSum")));
    ui->table_optHeader->setItem(20,0,new QTableWidgetItem(QString("Subsystem")));
    ui->table_optHeader->setItem(21,0,new QTableWidgetItem(QString("DllCharacteristics")));
    ui->table_optHeader->setItem(22,0,new QTableWidgetItem(QString("SizeOfStackReserve")));
    ui->table_optHeader->setItem(23,0,new QTableWidgetItem(QString("SizeOfStackCommit")));
    ui->table_optHeader->setItem(24,0,new QTableWidgetItem(QString("SizeOfHeapReserve")));
    ui->table_optHeader->setItem(25,0,new QTableWidgetItem(QString("SizeOfHeapCommit")));
    ui->table_optHeader->setItem(26,0,new QTableWidgetItem(QString("LoaderFlags")));
    ui->table_optHeader->setItem(27,0,new QTableWidgetItem(QString("NumberOfRvaAndSizes")));
    ui->table_optHeader->setItem(28,0,new QTableWidgetItem(QString("DataDirectory")));

    ui->table_optHeader->setItem(0,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->Magic,0,16)));
    ui->table_optHeader->setItem(1,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->MajorLinkerVersion,0,16)));
    ui->table_optHeader->setItem(2,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->MinorLinkerVersion,0,16)));
    ui->table_optHeader->setItem(3,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->SizeOfCode,0,16)));
    ui->table_optHeader->setItem(4,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->SizeOfInitializedData,0,16)));
    ui->table_optHeader->setItem(5,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->SizeOfUninitializedData,0,16)));
    ui->table_optHeader->setItem(6,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->AddressOfEntryPoint,0,16)));
    ui->table_optHeader->setItem(7,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->BaseOfCode,0,16)));
    ui->table_optHeader->setItem(8,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->BaseOfData,0,16)));
    ui->table_optHeader->setItem(9,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->ImageBase,0,16)));
    ui->table_optHeader->setItem(10,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->SectionAlignment,0,16)));
    ui->table_optHeader->setItem(11,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->FileAlignment,0,16)));
    ui->table_optHeader->setItem(12,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->MajorOperatingSystemVersion,0,16)));
    ui->table_optHeader->setItem(13,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->MinorOperatingSystemVersion,0,16)));
    ui->table_optHeader->setItem(14,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->MajorImageVersion,0,16)));
    ui->table_optHeader->setItem(15,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->MinorImageVersion,0,16)));
    ui->table_optHeader->setItem(16,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->Win32VersionValue,0,16)));
    ui->table_optHeader->setItem(17,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->SizeOfImage,0,16)));
    ui->table_optHeader->setItem(18,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->SizeOfHeaders,0,16)));
    ui->table_optHeader->setItem(19,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->CheckSum,0,16)));
    ui->table_optHeader->setItem(20,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->Subsystem,0,16)));
    ui->table_optHeader->setItem(21,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->DllCharacteristics,0,16)));
    ui->table_optHeader->setItem(22,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->SizeOfStackReserve,0,16)));
    ui->table_optHeader->setItem(23,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->SizeOfStackCommit,0,16)));
    ui->table_optHeader->setItem(24,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->SizeOfHeapReserve,0,16)));
    ui->table_optHeader->setItem(25,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->SizeOfHeapCommit,0,16)));
    ui->table_optHeader->setItem(26,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->LoaderFlags,0,16)));
    ui->table_optHeader->setItem(27,1,new QTableWidgetItem(QString("0x%1").arg(pOptHdr->NumberOfRvaAndSizes,0,16)));
    ui->table_optHeader->setItem(28,1,new QTableWidgetItem(QString("0x%1").arg((DWORD)pOptHdr->DataDirectory,0,16)));

    ui->table_optHeader->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_optHeader->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);


    // ---------------------------------------------------
    //                  解析数据目录表
    //
    // ---------------------------------------------------
    ui->table_dataDirectory->setColumnCount(2);
    ui->table_dataDirectory->setRowCount(15);
    header.clear();
    header<<"VirtualAddress"<<"Size";
    ui->table_dataDirectory->setHorizontalHeaderLabels(header);
    header.clear();
    header<<"0 Export Table" << "1 Import Table" << "2 Resources Table"
         << "3 Exception Table" << "4 Security Table" << "5 Base relocation Table"
         << "6 Debug"  << "7 Copyright"  << "8 Global Ptr" << "9 Thread local storge"
         << "10 Load configuration" << "11 Bound Import" << "12 Import Address Table"
         << "13 Delay Import" << "14 COM descriptor" << "15 Reserved";
    ui->table_dataDirectory->setVerticalHeaderLabels(header);

    IMAGE_DATA_DIRECTORY *img_data_dir;
    int nNum = 0;
    QString msg;
    for (img_data_dir = pOptHdr->DataDirectory; nNum <= 14; img_data_dir++,nNum++)
    {
        ui->table_dataDirectory->setItem(nNum,0,new QTableWidgetItem(QString("0x%1").arg(img_data_dir->VirtualAddress,0,16)));
        ui->table_dataDirectory->setItem(nNum,1,new QTableWidgetItem(QString("0x%1").arg(img_data_dir->Size,0,16)));
    }

    ui->table_dataDirectory->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_dataDirectory->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);



    // 解析区段头
    IMAGE_SECTION_HEADER* pScnHdr,*pScnHdrTemp;
    pScnHdr = IMAGE_FIRST_SECTION(pNtHdr);
    pScnHdrTemp = IMAGE_FIRST_SECTION(pNtHdr);

    // 得到区段的个数
    DWORD nScnCount = pFileHdr->NumberOfSections;
    ui->table_sectionTable->setColumnCount(1);
    ui->table_sectionTable->setRowCount(nScnCount);

    header.clear();

    header<<"Section";
    ui->table_sectionTable->setHorizontalHeaderLabels(header);

    for (int i = 0; i < nScnCount; ++i,++pScnHdrTemp)
    {
        msg=QString("%1").arg(QString((char*)(pScnHdrTemp->Name)));
        ui->table_sectionTable->setItem(i,0,new QTableWidgetItem(msg));
    }

    ui->table_sectionTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_sectionTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // -------------------------------------------------------------------------------------------------------------
    //
    //                                               导出表
    //
    // -------------------------------------------------------------------------------------------------------------
    // 得到导出表的的RVA
    IMAGE_DATA_DIRECTORY* pDataDir = pOptHdr->DataDirectory;
    DWORD dwExpTabRva =
        pDataDir[ IMAGE_DIRECTORY_ENTRY_EXPORT ].VirtualAddress;

    IMAGE_EXPORT_DIRECTORY* pExpTab =
        (IMAGE_EXPORT_DIRECTORY*)( RVAToOffset( pFileBuff , dwExpTabRva ) + (DWORD)pFileBuff );

    // 获取导出表中的dll的名字
    // dll名 = dll名的RVA -> dll名的文件偏移 -> dll名的文件偏移 + PE文件缓冲区的首地址
    char* pDllName =
    (char*)(RVAToOffset( pFileBuff , pExpTab->Name ) + (DWORD)pFileBuff);
    QString( "DLL: %1\n").arg(pDllName);
    qDebug()<<QString( "DLL: %1\n").arg(pDllName);

    // 获取导出地址表在文件中的位置
    DWORD dwAddrTabRva = pExpTab->AddressOfFunctions;
    // 导出地址表, 每个元素都是DWORD,因此,在这里使用DWORD*来保存表的首地址
    DWORD* pAddrTab =
        (DWORD*)( RVAToOffset( pFileBuff , dwAddrTabRva ) + (DWORD)pFileBuff );

    // 获取函数名, 需要找到函数名称地址表
    DWORD dwNameTabRva = pExpTab->AddressOfNames;
    DWORD* pNameTab =
        (DWORD*)( RVAToOffset( pFileBuff , dwNameTabRva ) + (DWORD)pFileBuff );


    // 如果需要获取地址对应的函数名,需要找到保存地址和序号映射信息的序号表.
    DWORD dwOrdinalRva = pExpTab->AddressOfNameOrdinals;
    // 由于序号表中的每个元素,都是WORD类型,因此,使用WORD*来保存表的首地址
    WORD* pOrdinalTab =
        (WORD*)( RVAToOffset( pFileBuff , dwOrdinalRva ) + (DWORD)pFileBuff );

    DWORD dwCount = pExpTab->NumberOfFunctions;

    ui->table_exportTable->setColumnCount(2);
    ui->table_exportTable->setRowCount(dwCount);
    header.clear();
    header<<"Address of functions" << "Name of functions";
    ui->table_exportTable->setHorizontalHeaderLabels(header);

    for(DWORD i=0; i<dwCount;i++)
    {
        QString addr = QString("0x%1").arg(pAddrTab[i],0,16);
        ui->table_exportTable->setItem(i,0,new QTableWidgetItem(addr));
    }

    DWORD dwNameCount = pExpTab->NumberOfNames;
    for(DWORD i=0; i< dwNameCount; i++)
    {
        DWORD dwNameRva = pNameTab[i];
        char* pName = (char*)(RVAToOffset(pFileBuff,dwNameRva) + (DWORD)pFileBuff);
        DWORD dwOrd = pOrdinalTab[i];
        QString name = QString(pName);
        ui->table_exportTable->setItem(dwOrd,1,new QTableWidgetItem(name));
    }

    ui->table_exportTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_exportTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // -------------------------------------------------------------------------------------------------------------
    //
    //                                               导入表
    //
    // -------------------------------------------------------------------------------------------------------------

    DWORD dwImpTabRva = pDataDir[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress;
        // 将RVA转文件偏移
    IMAGE_IMPORT_DESCRIPTOR* pImp =
            (IMAGE_IMPORT_DESCRIPTOR*)( RVAToOffset( pFileBuff , dwImpTabRva ) + (DWORD)pFileBuff );
    ui->tree_importTable->clear();
    ui->tree_importTable->setColumnCount(2);
    ui->tree_importTable->setHeaderLabels(QStringList()<<"ordinals"<<"name");
    //QTreeWidgetItem *root = new QTreeWidgetItem(ui->tree_importTable,QStringList(QString("import")));
    while(pImp->Characteristics != 0 )
    {
        pDllName = (char*)(RVAToOffset(pFileBuff, pImp->Name) + (DWORD)pFileBuff);
        QTreeWidgetItem *root = new QTreeWidgetItem(ui->tree_importTable,QStringList(QString(pDllName)));

        pImp->OriginalFirstThunk;// 保存的是INT的结构体数组的RVA
        pImp->FirstThunk; // 保存的是IAT结构体数组的RVA

        // 获取到INT数组
        IMAGE_THUNK_DATA32* pInt =
            ( IMAGE_THUNK_DATA32*)(RVAToOffset(pFileBuff,pImp->OriginalFirstThunk) + DWORD(pFileBuff));

        while( pInt->u1.AddressOfData) {

            if( IMAGE_SNAP_BY_ORDINAL( pInt->u1.Ordinal ) == TRUE ) {

                new QTreeWidgetItem(root,QStringList(QString("0x%1").arg(pInt->u1.Ordinal,0,16)));
            }
            else {
                // 否则就是以名称导入的
                IMAGE_IMPORT_BY_NAME* pImpName=
                    (IMAGE_IMPORT_BY_NAME*)
                    ( RVAToOffset( pFileBuff , pInt->u1.Function ) + (DWORD)pFileBuff );

                new QTreeWidgetItem(root,QStringList()<<QString("0x%1").arg(pImpName->Hint,0,16)<<QString("%1").arg((char*)(pImpName->Name)));
            }

            // 递增到下一个INT
            ++pInt;
        }
        ++pImp;
    }

    ui->tree_importTable->header()->setSectionResizeMode(QHeaderView::Stretch);

    // -------------------------------------------------------------------------------------------------------------
    //
    //                                               资源列表
    //
    // -------------------------------------------------------------------------------------------------------------

    //ui->tree_resource->setColumnCount();
    ui->tree_resource->clear();
    QTreeWidgetItem *root = new QTreeWidgetItem(ui->tree_resource,QStringList(QString("resource")));
    // 声明资源列表根位置
    IMAGE_RESOURCE_DIRECTORY* pResRoot;
    // 查询数据目录表，找到资源列表的偏移
    DWORD dwResRootRva = pDataDir[ IMAGE_DIRECTORY_ENTRY_RESOURCE ].VirtualAddress;
    // 将虚拟内存地址转化为文件偏移，然后加上文件的基地址，得到资源列表在文件中的位置
    pResRoot = (IMAGE_RESOURCE_DIRECTORY*)(RVAToOffset(pFileBuff,dwResRootRva) + (DWORD)pFileBuff);
    // 将根目录和需要解析的资源目录传入解析函数
    parseResourceTable((DWORD)pResRoot,pResRoot,1,(DWORD)pFileBuff,root);
    ui->tree_resource->header()->setSectionResizeMode(QHeaderView::Stretch);


    // -------------------------------------------------------------------------------------------------------------
    //
    //                                               重定位表
    //
    // -------------------------------------------------------------------------------------------------------------
    IMAGE_BASE_RELOCATION* pBaseRel;
    DWORD dwBaseRel = (DWORD)pDataDir[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
    pBaseRel = (IMAGE_BASE_RELOCATION*)(RVAToOffset(pFileBuff, dwBaseRel)+(DWORD)pFileBuff);

    ui->tree_reloc->clear();

    while(pBaseRel->VirtualAddress!=0)
    {
        // 得到重定位项的个数
        DWORD dwCount =
                (pBaseRel->SizeOfBlock - (sizeof(IMAGE_BASE_RELOCATION)))/sizeof(WORD);

        qDebug()<< "pBaseRel->VirtualAddress:" << QString("%1").arg(pBaseRel->VirtualAddress,0,16);
        QTreeWidgetItem* root = new QTreeWidgetItem(ui->tree_reloc,QStringList()<<QString("%1").arg(pBaseRel->VirtualAddress,0,16));

        // 得到重定位项的开始地址

        TypeOffset *pTypeOffset = (TypeOffset*)(pBaseRel + 1);
        for(DWORD i = 0; i < dwCount; i++)
        {
            if(pTypeOffset[i].Type == IMAGE_REL_BASED_HIGHLOW)
            {

                // 得到需要修复的RVA
                DWORD dwRva = pBaseRel->VirtualAddress + pTypeOffset[i].Offset;

                qDebug()<<"需要修复的RVA"<<QString("%1").arg(dwRva,0,16);

                DWORD dwOffset = RVAToOffset(pFileBuff, dwRva);

                // 得到需要修复的opcode地址操作数的所在地址
                DWORD* pAddress = (DWORD*)(dwOffset + (DWORD)pFileBuff);

                qDebug() << "需要修复的opcode地址"<<*pAddress;
                new QTreeWidgetItem(root,QStringList()<<QString("RVA:%1 -- Opcode:%2").arg(dwRva,0,16).arg(*pAddress,0,16));
            }
        }

        // 找到下一个重定位块
        pBaseRel = (IMAGE_BASE_RELOCATION*)
                ((DWORD)pBaseRel + pBaseRel->SizeOfBlock);
    }
    ui->tree_reloc->header()->setSectionResizeMode(QHeaderView::Stretch);


    // -------------------------------------------------------------------------------------------------------------
    //
    //                                               延迟加载表
    //
    // -------------------------------------------------------------------------------------------------------------

    IMAGE_DELAYLOAD_DESCRIPTOR* pDelayReLoad;
    DWORD dwDelayLoad = (DWORD)pDataDir[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress;
    pDelayReLoad = (IMAGE_DELAYLOAD_DESCRIPTOR*)(RVAToOffset(pFileBuff, dwDelayLoad)+(DWORD)pFileBuff);
    ui->tree_delayLoad->clear();


    if(dwDelayLoad != 0)
    {
        while(pDelayReLoad->DllNameRVA != 0)
        {

            char* name = (char*)(RVAToOffset(pFileBuff,pDelayReLoad->DllNameRVA) + DWORD(pFileBuff));

            QTreeWidgetItem *root = new QTreeWidgetItem(ui->tree_delayLoad,QStringList(QString(name)));

            // 获取到INT数组
            DWORD dwFunCount = 0;
            IMAGE_THUNK_DATA32* pInt =
                ( IMAGE_THUNK_DATA32*)(RVAToOffset(pFileBuff,pDelayReLoad->ImportNameTableRVA) + DWORD(pFileBuff));


            while( pInt->u1.AddressOfData) {

                if( IMAGE_SNAP_BY_ORDINAL( pInt->u1.Ordinal ) == TRUE ) {

                    qDebug() << QString("0x%1").arg(pInt->u1.Ordinal,0,16);
                    new QTreeWidgetItem(root,QStringList(QString("0x%1").arg(pInt->u1.Ordinal,0,16)));
                }
                else {
                    // 否则就是以名称导入的
                    IMAGE_IMPORT_BY_NAME* pImpName=
                        (IMAGE_IMPORT_BY_NAME*)
                        ( RVAToOffset( pFileBuff , pInt->u1.Function ) + (DWORD)pFileBuff );

                    qDebug() << QString("0x%1").arg(pImpName->Hint,0,16)<<QString("%1").arg((char*)(pImpName->Name));
                    new QTreeWidgetItem(root,QStringList()<<QString("0x%1 -- %2").arg(pImpName->Hint,0,16).arg((char*)(pImpName->Name)));
                }

                // 递增到下一个INT

                ++pInt;
            }
            pDelayReLoad++;
        }
    }

    // -------------------------------------------------------------------------------------------------------------
    //
    //                                               TLS表
    //
    // -------------------------------------------------------------------------------------------------------------

    IMAGE_TLS_DIRECTORY *pTlsDir;
    DWORD dwTlsDir = (DWORD)pDataDir[IMAGE_DIRECTORY_ENTRY_TLS ].VirtualAddress;
    pTlsDir = (IMAGE_TLS_DIRECTORY*)(RVAToOffset(pFileBuff, dwTlsDir)+(DWORD)pFileBuff);
    ui->table_tls->clear();

    if(dwTlsDir != 0)
    {
        ui->table_tls->setRowCount(6);
        header.clear();
        header<< "StartAddressOfRawData"
              << "EndAddressOfRawData"
              << "AddressOfIndex"
              << "AddressOfCallBacks"
              << "SizeOfZeroFill"
              << "Characteristics";

        ui->table_tls->setVerticalHeaderLabels(header);
        header.clear();
        header << "value";
        ui->table_tls->setColumnCount(1);
        ui->table_tls->setHorizontalHeaderLabels(header);

        ui->table_tls->setItem(0,0,new QTableWidgetItem(QString("0x%1").arg(pTlsDir->StartAddressOfRawData,0,16)));
        ui->table_tls->setItem(1,0,new QTableWidgetItem(QString("0x%1").arg(pTlsDir->EndAddressOfRawData,0,16)));
        ui->table_tls->setItem(2,0,new QTableWidgetItem(QString("0x%1").arg(pTlsDir->AddressOfIndex,0,16)));
        ui->table_tls->setItem(3,0,new QTableWidgetItem(QString("0x%1").arg(pTlsDir->AddressOfCallBacks,0,16)));
        ui->table_tls->setItem(4,0,new QTableWidgetItem(QString("0x%1").arg(pTlsDir->SizeOfZeroFill,0,16)));
        ui->table_tls->setItem(5,0,new QTableWidgetItem(QString("0x%1").arg(pTlsDir->Characteristics,0,16)));

        ui->table_tls->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        ui->table_tls->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }
}

void PE_Dialog::parseResourceTable(DWORD dwResRootDirAddr ,/*根目录的首地址*/
                        IMAGE_RESOURCE_DIRECTORY* pResDir ,/*需要解析的资源目录*/
                        int nDeep /*记录这是第几层目录*/,
                        DWORD pFileBuff,
                        QTreeWidgetItem* leaf)
{
    // 获取资源目录入口的个数
    DWORD dwEntryCount = pResDir->NumberOfIdEntries + pResDir->NumberOfNamedEntries;


    // 获取目录入口数组的首地址
    IMAGE_RESOURCE_DIRECTORY_ENTRY* pResDirEntry;
    pResDirEntry = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(pResDir + 1);

    // 声明缓冲区
    WCHAR buff[512];
    QTreeWidgetItem* child;

    if(nDeep == 1) // 第一层
    {
        // 第一层目录入口，ID信息是资源类型ID
        // 指向下一层的偏移，指向的是第二层的资源目录
        for(DWORD i=0; i< dwEntryCount; i++)
        {
            // 判断资源类型
            // 判断资源类型是整形的还是字符串类型的ID
            if(pResDirEntry[i].NameIsString == 1)
            {
                // 资源类型是字符串时
                IMAGE_RESOURCE_DIR_STRING_U* pTypeName;
                pTypeName = (IMAGE_RESOURCE_DIR_STRING_U*)
                        (pResDirEntry[i].NameOffset + dwResRootDirAddr);
                qDebug()<<"第一层 - 资源类型："<<pTypeName->NameString;
                QString str = QString::fromWCharArray(pTypeName->NameString);
                child =  new QTreeWidgetItem(leaf,QStringList(str));
            }
            else
            {
                // 说明资源类型是一个整形的值
                QStringList szType = QStringList()
                        <<QString("")<<QString("光标")
                        <<QString("位图")<<QString("图标")
                        <<QString("菜单")<<QString("对话框")
                        <<QString("字符串表")<<QString("字体目录")
                        <<QString("字体")<<QString("加速键")
                        <<QString("RC数据")<<QString("消息表")
                        <<QString("光标组")<<QString("")
                        <<QString("图标组")<<QString("")
                        <<QString("版本信息")<<QString("对话框包含目录")
                        <<QString("")<<QString("")<<QString("")<<QString("")<<QString("")
                        <<QString("HTML")<<QString("清单文件");

                if(pResDirEntry[i].Id >= 1 && pResDirEntry[i].Id <= 24)
                {
                    //qDebug()<<pResDirEntry[i].Id;
                    qDebug()<<"第一层 - 资源类型："<<szType.at(pResDirEntry[i].Id);
                    child =  new QTreeWidgetItem(leaf,QStringList(QString(szType.at(pResDirEntry[i].Id))));

                }
                else
                {
                    qDebug()<<"第一层 - 资源类型："<<pResDirEntry[i].Id;
                    child =  new QTreeWidgetItem(leaf,QStringList(QString("0x%1").arg(pResDirEntry[i].Id,0,16)));
                }
            }

            // 解析下一层目录
            IMAGE_RESOURCE_DIRECTORY* pNextDir =
                    (IMAGE_RESOURCE_DIRECTORY*)(pResDirEntry[i].OffsetToDirectory + dwResRootDirAddr);
            parseResourceTable(dwResRootDirAddr, pNextDir, nDeep + 1,(DWORD)pFileBuff,child);
        }
    }
    else if(nDeep == 2) // 第二层
    {
        for(DWORD i=0; i<dwEntryCount;i++)
        {
            //资源目录的第二层，保存的是各种资源的资源ID
            // 解析ID
            // 1.整形ID
            // 2.字符串ID
            if(pResDirEntry[i].NameIsString)
            {
                // 资源类型是字符串
                IMAGE_RESOURCE_DIR_STRING_U* pTypeName;
                // NameOffset 保存的偏移，是把资源根目录的地址作为基地址的偏移
                pTypeName =
                        (IMAGE_RESOURCE_DIR_STRING_U*)
                        (pResDirEntry[i].NameOffset + dwResRootDirAddr);
                qDebug()<<pTypeName;
                QString str = QString::fromWCharArray(pTypeName->NameString);
                child =  new QTreeWidgetItem(leaf,QStringList(str));
            }
            else
            {
                qDebug()<<pResDirEntry[i].Id;
                child =  new QTreeWidgetItem(leaf,QStringList(QString("0x%1").arg(pResDirEntry[i].Id,0,16)));
            }

            // 解析偏移
            // 解析下一层目录
            IMAGE_RESOURCE_DIRECTORY* pNextDir =
                    (IMAGE_RESOURCE_DIRECTORY*)(pResDirEntry[i].OffsetToDirectory + dwResRootDirAddr);
            parseResourceTable(dwResRootDirAddr, pNextDir, nDeep + 1,(DWORD)pFileBuff,child);
        }
    }

    else if(nDeep == 3)
    {
        IMAGE_RESOURCE_DATA_ENTRY* pResDataEntry = 0;
        if(pResDirEntry->DataIsDirectory == 0)
        {
            /* 获得了数据所在的RVA */
            qDebug()<<"pFileBuff"<<pFileBuff;
            pResDataEntry = (IMAGE_RESOURCE_DATA_ENTRY*)(pResDirEntry->OffsetToData + dwResRootDirAddr);
            // 得到资源的内存地址
            LPVOID pData = (LPVOID)(RVAToOffset((LPVOID)pFileBuff,(DWORD)(pResDataEntry->OffsetToData)) + (DWORD)pFileBuff);
            qDebug()<<pResDataEntry->Size<<pData;
            new QTreeWidgetItem(leaf,QStringList(QString("0x%1").arg((DWORD)pData,0,16)));
        }
    }
}
