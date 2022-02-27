/*
 *  cutsrt Ver.0.01     Copyright (C) 2009  kt
 *  cutass Ver.0.10     (cutsrt mod  2011-2012)
 */
#define PROGRAM_VERSION    "0.10"

#include <ctype.h>
#include <stdio.h>
#include <vector>

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <sstream>
#include <cstring>

using namespace std;

typedef unsigned char  byte;
typedef unsigned short ushort;
typedef unsigned long  ulong;
typedef unsigned int   uint;
typedef unsigned long long uint64;
typedef long long int64;

#define _MAX_PATH   260 /* max. length of full pathname */
#define _MAX_DRIVE  3   /* max. length of drive component */
#define _MAX_DIR    256 /* max. length of path component */
#define _MAX_FNAME  256 /* max. length of file name component */
#define _MAX_EXT    256 /* max. length of extension component */

/**** Constant define ****/
#define    SZ_PATH_MAX       (256)
 
#define    FLG_EXTENSION     (1)
#define    FLG_FILENAME      (2)
#define    FLG_DIRNAME       (4)
#define    FLG_BASENAME      (8)
#define    FLG_WILDCARDS     (0x10)
#define    FLG_ABSPATH       (0x20)

typedef enum {
    CUT_KEY_A,
    CUT_KEY_1,
    CUT_KEY_2,
    CUT_KEY_3,
    CUT_AVS,
    CUT_VCF,
    CUT_DEL
} cut_list_type;

static int fps;
static int delay_time;
static int ts_read_start_delay;
static ushort select_pmt_pid;
static int d2v_start_offset;
static int check_gop_header_num;
static std::vector<int> dellist;
static bool side_cut;
static int sidebar_size;
static int shiftX, shiftY;

int load_dellist( const char *filename, cut_list_type cut_type );
double get_cut_frame_number( double frame, int mode );
void cut_ass( const char *file_in, const char *file_out );
void cut_srt( const char *file_in, const char *file_out );
void calc_numstring( char* buf, const int buf_size );
int parse_d2v_info( const char *file_in, int &start_offset, char *fin_path );
int parse_transport_stream( const char *file_in, int mode );
void _splitpath(const char *path,char *drive, char *dirpos, char *basepos, char *extpos);

double time2frame( double time );
double frame2time( double frame );



#include <stdarg.h>
typedef int (* debug_print)( FILE *stream, const char *format, ... );

static debug_print debug_print_log = NULL;
static debug_print debug_print_low = NULL;
static debug_print debug_print_err = NULL;

int show_info_silent( FILE *stream, const char *format, ... )
{
    return 0;
}
int show_info_normal( FILE *stream, const char *format, ... )
{
    va_list argptr;
    va_start( argptr, format );
    vfprintf( stream, format, argptr );
    va_end( argptr );
    return 0;
}
/*!
 * 様々な型のstringへの変換(stringstreamを使用)
 * @param[in] x 入力
 * @return string型に変換したもの
 */
template<typename T>
inline string RX_TO_STRING(const T &x)
{
    stringstream ss;
    ss << x;
    return ss.str();
}
//! string型に<<オペレータを設定
template<typename T>
inline string &operator<<(string &cb, const T &a)
{
    cb += RX_TO_STRING(a);
    return cb;
}
/*!
 * パスからファイル名のみ取り出す
 * @param[in] path パス
 * @return ファイル名
 */
inline string GetFileName(const string &path)
{
    size_t pos1;
 
    pos1 = path.rfind('\\');
    if(pos1 != string::npos){
        return path.substr(pos1+1, path.size()-pos1-1);
    }
 
    pos1 = path.rfind('/');
    if(pos1 != string::npos){
        return path.substr(pos1+1, path.size()-pos1-1);
    }
 
    return path;
}
/*!
 * パスからファイル名を取り除いたパスを抽出
 * @param[in] path パス
 * @return フォルダパス
 */
inline string GetFolderPath(const string &path)
{
    size_t pos1;
 
    pos1 = path.rfind('\\');
    if(pos1 != string::npos){
        return path.substr(0, pos1+1);
        
    }
 
    pos1 = path.rfind('/');
    if(pos1 != string::npos){
        return path.substr(0, pos1+1);
    }
 
    return "";
}
/*!
 * パスからファイルの親フォルダ名を取り出す
 * @param[in] path ファイルパス
 * @return 親フォルダ名
 */
inline string GetParentFolderName(const string &path)
{
	std::string::size_type pos1, pos0;
	pos1 = path.find_last_of("\\/");
	pos0 = path.find_last_of("\\/", pos1-1);
 
	if(pos0 != std::string::npos && pos1 != std::string::npos){
		return path.substr(pos0+1, pos1-pos0-1);
	}
	else{
		return "";
	}
}
/*!
 * パスから拡張子を小文字にして取り出す
 * @param[in] path ファイルパス
 * @return (小文字化した)拡張子
 */
inline string GetExtension(const string &path)
{
    string ext;
    size_t pos1 = path.rfind('.');
    if(pos1 != string::npos){
        ext = path.substr(pos1+1, path.size()-pos1);
        string::iterator itr = ext.begin();
        while(itr != ext.end()){
            *itr = tolower(*itr);
            itr++;
        }
        itr = ext.end()-1;
        while(itr != ext.begin()){    // パスの最後に\0やスペースがあったときの対策
            if(*itr == 0 || *itr == 32){
                ext.erase(itr--);
            }
            else{
                itr--;
            }
        }
    }
 
    return ext;
}
/*!
 * ファイル名から拡張子を削除
 * @param[in] fn ファイル名(フルパス or 相対パス)
 * @return フォルダパス
 */
inline string ExtractPathWithoutExt(const string &fn)
{
    string::size_type pos;
    if((pos = fn.find_last_of(".")) == string::npos){
        return fn;
    }
 
    return fn.substr(0, pos);
}
string ExtractFileName(const string &path, bool without_extension = true)
{
    string fn;
    string::size_type fpos;
    if((fpos = path.find_last_of("/")) != string::npos){
        fn = path.substr(fpos+1);
    }
    else if((fpos = path.find_last_of("\\")) != string::npos){
		fn = path.substr(fpos+1);
	}
	else{
		fn = path;
	}
 
	if(without_extension && (fpos = fn.find_last_of(".")) != string::npos){
		fn = fn.substr(0, fpos);
	}
 
	return fn;
}
void _splitpath(const char *path,char *drive, char *dirpos, char *basepos, char *extpos)
{
    string strpath , dname , bname , ename ;
    strpath = RX_TO_STRING(path);
    dname = GetFolderPath(strpath);
    bname = ExtractFileName(strpath,true);
    ename = GetExtension(strpath);
    std::strcpy(dirpos, dname.c_str());
    std::strcpy(basepos, bname.c_str());
    std::strcpy(extpos, ".");
    std::strcat(extpos, ename.c_str());
}
void _makepath( char *fin_path, char *drive, char *dir, char *fname, char *fext){
    memset( fin_path, 0, sizeof( fin_path ) );
    strcat(fin_path, dir);
    strcat(fin_path, fname);
    if (fext != NULL ){
        strcat(fin_path, fext);
    }
}
void reverse(char *s){
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}
void _itoa( int n, char *s, int digit ){
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % digit + '0';   /* get next digit */
     } while ((n /= digit) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
}
static void usage( void )
{
    const char *msg =
        "cutsrt Ver.0.01  Copyright (C) 2009  kt\n"
        "cutass Ver." PROGRAM_VERSION "  (cutsrt mod  2011-2012)\n"
        "\n"
        "usage:\n"
        "  cutass [<options>] <input{.ass|.srt}>\n"
        "\n"
        "options:\n"
        "  -l <AviUtl_DelList.txt>      default: input_del.txt\n"
        "  -t <trimfile.avs>\n"
        "  -v <vcffile.vcf>\n"
        "  -k{1|2|3} <TMPGEnc.keyframe>\n"
        "  -o <output{.ass|.srt}>       default: input_new{.ass|.srt}\n"
        "  -fps <frame_rate_value>      default: 30\n"
        "  -delay <msec>\n"
        "  -md              : parse tsfile.     [for DGIndex]\n"
        "  -mv              : parse tsfile.     [for MPEG-2 VIDEO VFAPI Plug-In]\n"
        "  -mt              : parse tsfile.     [for TMPGEnc]\n"
        "  -pmt_pid <PID>   : select PMT PID\n"
        "  -gop <start_offset>                  [DGIndex only]\n"
        "  -debug  : show debug info.\n"
        "  -silent : show error only.\n"
        " [extra option]\n"
        "  -sc <sidebar_size>       [for Side cut] Changed to ""4:3"" from ""16:9""\n"
        "  -sx <offset_X_size>\n"
        "  -sy <offset_Y_size>\n"
    ;
    fputs( msg, stderr );
}

void init()
{
    fps = 30;
    delay_time = 0;
    ts_read_start_delay = 0;
    select_pmt_pid = 0x0000;
    d2v_start_offset = 0;
    check_gop_header_num = -1;
    debug_print_log = &show_info_normal;
    debug_print_low = &show_info_silent;
    debug_print_err = &show_info_normal;
    side_cut = false;
    sidebar_size = 0;
    shiftX = shiftY = 0;
    dellist.clear();
}

int main( int argc, char *argv[] )
{
    char buf[_MAX_PATH], buf2[_MAX_PATH];
    char *listfile = NULL, *infile = NULL, *outfile = NULL;
    cut_list_type cut_type = CUT_DEL;
    int ts_parse_mode = -1;
    init();

    while( *++argv )
    {
        char c = (*argv)[0];
        if( c == '-' )
        {
            switch( tolower( (*argv)[1] ) )
            {
                case 'g':
                    if( !strcmp( (*argv),"-gop" ) )
                    {
                        check_gop_header_num = atoi( *++argv );
                        if( check_gop_header_num <= 0 )
                            check_gop_header_num = 0;
                    }
                    break;
                case 'm':
                    if( !strcmp( (*argv), "-md" ) )
                        ts_parse_mode = 0;
                    else if( !strcmp( (*argv), "-mv" ) )
                        ts_parse_mode = 1;
                    else if( !strcmp( (*argv), "-mt") )
                        ts_parse_mode = 2;
                    break;
                case 'p':
                    if( !strcmp( (*argv), "-pmt_pid" ) )
                    {
                        strcpy( buf2, "0x" );
                        strcat( buf2, (*++argv) );
                        select_pmt_pid = strtol( buf2, NULL, 16 );
                    }
                    break;
                case 'f':
                    if( !strcmp( (*argv), "-fps" ) )
                    {
                        fps = atoi( *++argv );
                        if( fps <= 0 )
                            fps = 30;
                    }
                    break;
                case 'k':
                    if( !strcmp( (*argv), "-k1" ) )
                        cut_type = CUT_KEY_1;
                    else if( !strcmp( (*argv), "-k2" ) )
                        cut_type = CUT_KEY_2;
                    else if( !strcmp( (*argv), "-k3" ) )
                        cut_type = CUT_KEY_3;
                    else
                        cut_type = CUT_KEY_A;
                    listfile = *++argv;
                    break;
                case 't':
                    listfile = *++argv;
                    cut_type = CUT_AVS;
                    break;
                case 'v':
                    listfile = *++argv;
                    cut_type = CUT_VCF;
                    break;
                case 'l':
                    listfile = *++argv;
                    cut_type = CUT_DEL;
                    break;
                case 'o':
                    outfile = *++argv;
                    break;
                case 's':
                    if( !strcmp( (*argv), "-silent" ) )
                    {
                        debug_print_log = &show_info_silent;
                        debug_print_low = &show_info_silent;
                    }
                    else if( !strcmp( (*argv), "-sc" ) )
                    {
                        side_cut = true;
                        sidebar_size = atoi( *++argv );
                        if( sidebar_size <= 0 )
                        {
                            sidebar_size = 0;
                            --argv;
                        }
                    }
                    else if( !strcmp( (*argv), "-sx" ) )
                    {
                        shiftX = atoi( *++argv );
                    }
                    else if( !strcmp( (*argv), "-sy" ) )
                    {
                        shiftY = atoi( *++argv );
                    }
                    break;
                case 'd':
                    if( !strcmp( (*argv), "-delay" ) )
                        delay_time = atoi( *++argv );
                    else if( !strcmp( (*argv), "-debug") )
                    {
                        debug_print_log = &show_info_normal;
                        debug_print_low = &show_info_normal;
                    }
                    break;
//                case 'h':
//                    usage();
//                    break;
                default:
                    break;
            }
            if( !*argv )
                break;
        }
        else
        {
            infile = *argv;

            char fin_path[_MAX_PATH], ts_path[_MAX_PATH], existcheck_buf[_MAX_PATH];
            char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], fext[_MAX_EXT];

            _splitpath( infile, drive, dir, fname, fext );
            bool no_fext = ( strcmp( fext, ".ass" ) && strcmp( fext, ".srt" )
                            && strcmp( fext, ".ts" ) && strcmp( fext, ".d2v" ) ) ? true : false ;
            _makepath( fin_path, drive, dir, fname, ( no_fext ) ? fext : NULL );

            if( !listfile )
            {
                listfile = buf;
                _makepath( listfile, drive, dir, fname, NULL );
                strcat( listfile, "_del.txt" );
                cut_type = CUT_DEL;
            }
            if( !outfile )
            {
                outfile = buf2;
                _makepath( outfile, drive, dir, fname, ( no_fext ) ? fext : NULL );
                strcat( outfile, "_new" );
            }
            else
            {
                _splitpath( outfile, drive, dir, fname, fext );
                no_fext = ( strcmp( fext, ".ass" ) && strcmp( fext, ".srt" ) ) ? true : false;
                outfile = buf2;
                _makepath( outfile, drive, dir, fname, ( no_fext ) ? fext : NULL );
            }

            if( load_dellist( listfile, cut_type ) >= 0 )
            {
                int parse_result = 0;
                debug_print_log( stderr, "   ---- param option ----\n" );
                debug_print_log( stderr, "\tdelay time  : %d msec\n", delay_time );
                if( ts_parse_mode >= 0 )
                {
                    int parse_result = -1;
                    if( !strcmp( fext,".d2v" ) /*&& check_gop_header_num < 0*/ )
                        parse_result = parse_d2v_info( infile, d2v_start_offset, ts_path );
                    if( parse_result < 0 )
                        strcpy( ts_path, fin_path );
                    parse_result = parse_transport_stream( ts_path, ts_parse_mode );

                    FILE *check_fp;
                    strcpy( existcheck_buf, ts_path );
                    strcat( existcheck_buf, ".ass" );
                    if( (check_fp = fopen( existcheck_buf, "rt" )) )
                    {
                        strcpy( fin_path, ts_path );
                        fclose( check_fp );
                    }
                    else
                    {
                        strcpy( existcheck_buf, ts_path );
                        strcat( existcheck_buf, ".srt" );
                        if( (check_fp = fopen( existcheck_buf, "rt" )) )
                        {
                            strcpy( fin_path, ts_path );
                            fclose( check_fp );
                        }
                    }
                }
                debug_print_log( stderr, "\n" );
                if( parse_result >= 0 )
                {
                    if( !strcmp( fext, ".ass" ) )
                        cut_ass( fin_path, outfile );
                    else if( !strcmp( fext,".srt" ) )
                        cut_srt( fin_path, outfile );
                    else /* if( !strcmp( fext, "" ) ) */
                    {
                        cut_ass( fin_path, outfile );
                        cut_srt( fin_path, outfile );
                    }
                }
            }
            else if( delay_time && listfile == buf )
            {
                dellist.push_back( -1 );        // sentinel
                dellist.push_back( -1 );
                dellist.push_back( INT_MAX );
                dellist.push_back( -1 );        // sentinel
                dellist.push_back( -1 );        // sentinel

                debug_print_log( stderr, "\nOnly adaptation of delay is performed\n  delay:  " );
                if( !strcmp( fext, ".ass" ) )
                {
                    delay_time = (delay_time / 10) * 10;        // Floor
                    debug_print_log( stderr, "[ass] %d msec", delay_time );
                    cut_ass( fin_path, outfile );
                }
                else if( !strcmp( fext, ".srt" ) )
                {
                    debug_print_log( stderr, "[srt] %d msec", delay_time );
                    cut_srt( fin_path, outfile );
                }
                else /* if( !strcmp( fext, "" ) ) */
                {
                    debug_print_log( stderr, "[srt] %d msec    ", delay_time );
                    cut_srt( fin_path, outfile );
                    delay_time = (delay_time / 10) * 10;        // Floor
                    debug_print_log( stderr, "[ass] %d msec", delay_time );
                    cut_ass( fin_path, outfile );
                }
                debug_print_log( stderr, "\n\n" );

            }
            else
                debug_print_err( stderr, "list file open error: %s\n", listfile );
            listfile = outfile = NULL;
            cut_type = CUT_DEL;
            ts_parse_mode = -1;
            init();
        }
    }

    if( !infile )
        usage();

    return 0;
}

int load_dellist( const char *filename, cut_list_type cut_type )
{
    dellist.clear();

    FILE *fp = fopen( filename, "rt" );
    if( !fp )
        return -1;

    char buf[256];
    int result = 0;
    dellist.push_back( -1 );          // sentinel

    int start = 0, end = 0;
    int i = 0, j, c;
    int trim_pos[2];
    fpos_t fpos;
    int cutframes = 0, outframes = 0;       // for debug

    if( cut_type == CUT_KEY_A )
    {
        int odd_frame = 0;
        int even_frame = 0;
        if( fgets( buf, sizeof( buf ), fp ) )
        {
            trim_pos[i] = atoi( buf );
            i++;
            while( fgets( buf, sizeof( buf ), fp ) )
            {
                trim_pos[i] = atoi( buf );
                i ^= 1;
                if( i == 0 )
                    odd_frame += trim_pos[1] - trim_pos[0];
                else
                    even_frame += (trim_pos[0] - 1) - (trim_pos[1] + 1);
            }
            if( odd_frame <= even_frame )
                cut_type = CUT_KEY_1;
            else
                cut_type = CUT_KEY_2;
            debug_print_low( stderr, " [key]\to:%d\te:%d\t >> auto select:%d\n", odd_frame, even_frame, cut_type );
        }
        i = 0;
        fseek( fp, 0, SEEK_SET );
    }

    char *fread_result;
    switch( cut_type )
    {
    case CUT_KEY_A:
        result = -1;
        debug_print_err( stderr, " [key]\tAuto: Err!! Not select Odd/Even.\n\n" );
        break;
    case CUT_KEY_1:
        debug_print_log( stderr, " [key]\t1 : Odd\n" );

        if( fgets( buf, sizeof( buf ), fp ) )
        {
            while( fgets( buf, sizeof( buf ), fp ) )
            {
                trim_pos[i] = atoi( buf );
                i ^= 1;
                if( i == 0 )
                {
                    cutframes -= end;           // for debug

                    start = trim_pos[0];
                    end   = trim_pos[1];
                    dellist.push_back( start );
                    dellist.push_back( end );

                    debug_print_low( stderr, "\t  cut:\ts=%d\te=%d\n", start + 1, end - 1 );
                    outframes += (end - start - 1);     // for debug
                    cutframes += (start + 1);           // for debug
                }
            }
        }
        debug_print_low( stderr, "\n" );
        break;
    case CUT_KEY_2:
        debug_print_log( stderr, " [key]\t2 : Even\n" );

        if( fgets( buf, sizeof( buf ), fp ) )
        {
            trim_pos[i] = atoi( buf ) - 1;
            i++;
            while( fgets( buf, sizeof( buf ), fp ) )
            {
                trim_pos[i] = atoi( buf );
                i ^= 1;
                if( i == 0 )
                {
                    cutframes -= end;           // for debug

                    start = trim_pos[0];
                    end   = trim_pos[1];
                    dellist.push_back( start );
                    dellist.push_back( end );

                    debug_print_low( stderr, "\t  cut:\ts=%d\te=%d\n", start + 1, end - 1 );
                    outframes += (end - start - 1);      // for debug
                    cutframes += (start +1);            // for debug
                }
            }
        }
        debug_print_low( stderr, "\n" );
        break;
    case CUT_KEY_3:
        end = -1;           // for debug
        debug_print_log( stderr, " [key]\t3\n" );

        while( fgets( buf, sizeof( buf ), fp ) )
        {
            trim_pos[i] = atoi( buf );
            i ^= 1;
            if( i == 0 )
            {
                cutframes -= end;       // for debug

                start = trim_pos[0];
                end   = trim_pos[1];
                dellist.push_back( start - 1 );
                dellist.push_back( end + 1 );

                debug_print_low( stderr, "\t  cut:\ts=%d\te=%d\n", start, end );
                outframes += (end - start + 1);     // for debug
                cutframes += (start - 1);           // for debug
            }
        }
        debug_print_low( stderr, "\n" );
        break;
    case CUT_AVS:
        end = -1;        // for debug
        debug_print_log( stderr, " [avs]\n" );

        while( (c = fgetc( fp )) != EOF )
        {
            if( c == '#' )
            {
                if( !fgets( buf, sizeof( buf ), fp ) )
                    continue;
            }
            else if( c == '/' )
            {
                if( (c = fgetc( fp )) != EOF )
                    if( c == '*' )
                        while( (c = fgetc( fp )) != EOF )
                        {
check_commnet_end_1:
                            if( c =='*' )
                                if( (c = fgetc( fp )) != EOF )
                                {
                                    if( c == '/' )
                                    {
                                        break;
                                    }
                                    goto check_commnet_end_1;
                                }
                        }
            }
            else if( c == 'T' )
            {
                fgetpos( fp, &fpos );
                if( 3 == fread( buf, sizeof( char ), 3, fp ) )
                {
                    if( !strncmp( buf, "rim", 3 ) )
                    {
                        i = j = 0;
                        while( (c = fgetc( fp )) != EOF )
                        {
                            if( c == '(' )
                            {
                                int pahlen_num = 1;
                                while( (c = fgetc( fp )) != EOF )
                                {
                                    if( c == '/' )
                                        if( (c = fgetc( fp )) != EOF )
                                        {
                                            if( c == '*' )
                                            {
                                                while( (c = fgetc( fp )) != EOF )
                                                {
check_commnet_end_2:
                                                    if( c =='*' )
                                                        if( (c = fgetc( fp )) != EOF )
                                                        {
                                                            if( c == '/' )
                                                            {
                                                                break;
                                                            }
                                                            goto check_commnet_end_2;
                                                        }
                                                }
                                                continue;
                                            }
                                            buf[i] = '/';
                                            i++;
                                        }

                                    if( ('0' <= c && c <= '9') || c == '-' || c == '+' || c == '*' || c == '%' )
                                    {
                                        buf[i] = c;
                                        i++;
                                    }
                                    else if( c == ',' )
                                    {
                                        buf[i] = '\0';
                                        calc_numstring( buf, i );
                                        trim_pos[j] = atoi( buf );
                                        if( trim_pos[j] < 0 )
                                            trim_pos[j] = 0;

                                        i = 0;
                                        j++;
                                    }
                                    else if( '(' == c )
                                    {
                                        buf[i] = c;
                                        i++;
                                        pahlen_num++;
                                    }
                                    else if( c == ')' )
                                    {
                                        if( --pahlen_num > 0 )
                                        {
                                            buf[i] = c;
                                            i++;
                                            continue;
                                        }
                                        cutframes -= end;       // for debug

                                        buf[i] = '\0';
                                        calc_numstring( buf, i );
                                        trim_pos[j] = atoi( buf );

                                        start = trim_pos[0];
                                        end   = trim_pos[1];
                                        end = ( end < 0 ) ? start - end - 1 : end;
                                        dellist.push_back( start - 1 );
                                        dellist.push_back( end + 1 );

                                        debug_print_low( stderr, "\t Trim:\ts=%d\te=%d\n", start, end );
                                        outframes += (end - start + 1);     // for debug
                                        cutframes += (start - 1);           // for debug
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                    }
                    else
                        fsetpos( fp, &fpos );
                }
            }
        }
        debug_print_low( stderr, "\n" );
        break;
    case CUT_VCF:
        debug_print_log( stderr, " [vcf]\n" );

        while( fgets( buf, sizeof( buf ), fp ) )
        {
            cutframes -= (start + end);     // for debug

            if( sscanf( buf, "VirtualDub.subset.AddRange(%d,%d);", &start, &end ) == 2 )
            {
                dellist.push_back( start - 1 );
                dellist.push_back( start + end );

                debug_print_low( stderr, "\tRange:\ts=%d\te=%d\n", start, start + end - 1 );
                outframes += end;           // for debug
                cutframes += start;         // for debug
            }
        }
        cutframes += (start + end);         // for debug
        debug_print_low( stderr, "\n" );
        break;
    case CUT_DEL:
        start = end = -1;
        while( (fread_result = fgets( buf, sizeof( buf ), fp )) )
        {
            if( buf[0] == '#' )
                continue;
            if( sscanf( buf, "%d * %d", &start, &end ) == 2 )
                break;
        }
        if( !fread_result || start == -1 || end == -1 )
        {
            debug_print_err( stderr, "list file read error\n" );
            result = -1;
            break;
        }

        debug_print_log( stderr, " [del]\n" );

        debug_print_low( stderr, "\t list:\ts=%d", start );
        outframes = end + 1;        // for debug
        cutframes = start;          // for debug

        dellist.push_back( start - 1 );
        while( fgets( buf, sizeof( buf ), fp ) )
        {
            int del_start = -1, del_end = -1;
            if( buf[0] == '#' )
                continue;
            if( sscanf( buf, "%d * %d", &del_start, &del_end ) == 2 )
                continue;           // skip: duplicate "*"
            if( sscanf( buf, "%d - %d", &del_start, &del_end ) > 0 )
            {
                if( del_end == -1 )
                    del_end = del_start;        // case: 1frame cut

                dellist.push_back( del_start );
                dellist.push_back( del_end );

                debug_print_low( stderr, "\te=%d\n", del_start - 1 );
                debug_print_low( stderr, "\t list:\ts=%d", del_end + 1 );
                cutframes += (del_end - del_start + 1);     // for debug
            }
        }
        dellist.push_back(end + 1);

        debug_print_low( stderr, "\te=%d\n", end );
        debug_print_low( stderr, "\n" );
        outframes -= cutframes;         // for debug
        break;
    default:
        result = -1;
        break;
    }
    dellist.push_back( -1 );            // sentinel
    dellist.push_back( -1 );            // sentinel        fix
    fclose( fp );


    int t_time = frame2time( outframes );
    int c_time = frame2time( cutframes );
    int th, tm, ts, tms;
    int ch, cm, cs, cms;

    tms = t_time % 1000;
    t_time /= 1000;
    ts = t_time % 60;
    t_time /= 60;
    tm = t_time % 60;
    t_time /= 60;
    th = t_time % 60;

    cms = c_time % 1000;
    c_time /= 1000;
    cs = c_time % 60;
    c_time /= 60;
    cm = c_time % 60;
    c_time /= 60;
    ch = c_time % 60;

    debug_print_log( stderr, "   ---- cut info ----\n" );
    debug_print_log( stderr, "\tTotal  frame: %d  \ttime: %02d:%02d:%02d.%03d\n", outframes, th, tm, ts, tms );
    debug_print_log( stderr, "\t cut   frame: %d  \ttime: %02d:%02d:%02d.%03d\n", cutframes, ch, cm, cs, cms );
    debug_print_log( stderr, "\n");

    return result;
}

double get_cut_frame_number( double frame, int mode )
{
    int cutframes = -1;
    for( uint i = 0; i < dellist.size() - 1; i += 2 )
    {
// fix (no Used)
//        if( dellist[i+1] < 0 )
//            break;
        int oldcutframes = cutframes;
        cutframes += dellist[i+1] - dellist[i] + 1;
        if( dellist[i] <= frame && frame <= dellist[i+1] )
        {
            // deleted
            if( mode > 0 )
                return dellist[i+1] + 1 - cutframes;
            else if( mode < 0 )
                return dellist[i] - 1 - oldcutframes;
            else
                return -1;
        }
        else if( (dellist[i+1] < frame) && (frame < dellist[i+2]) )
        {
            // vaild
            return frame - cutframes;
        }
    }
    // not found
    if( mode < 0 )
    {
//        return dellist[dellist.size() - 1];
        // need start info:  start >= 0
        //  --> check, call before
        return (-cutframes);        // fix
    }
    return -1;
}


double time2frame( double time )
{
    return time * fps / 1001;    // fps
}

double frame2time( double frame )
{
    return frame * 1001 / fps;    // fps
}


void cut_ass( const char *file_in, const char *file_out )
{
    char fin_path[_MAX_PATH], fout_path[_MAX_PATH];
    strcpy( fin_path, file_in );
    strcat( fin_path, ".ass" );
    strcpy( fout_path, file_out );
    strcat( fout_path, ".ass" );

    FILE *fpin = fopen( fin_path, "rt" );
    if( !fpin )
    {
        debug_print_err( stderr, "input ass open error: %s\n", file_in );
        return;
    }
    FILE *fpout = fopen( fout_path, "wt" );
    if( !fpout )
    {
        debug_print_err( stderr, "output ass open error: %s\n", file_out );
        fclose( fpin );
        return;
    }

    char buf[256];
    char *ass_str, *ass_str_pos;
    while( fgets( buf, sizeof( buf ), fpin ) )
    {
        int PlayResX;
        int sh, sm, ss, sms;
        int eh, em, es, ems;
        int Layer;
        if( side_cut && sscanf( buf, "PlayResX: %d,", &PlayResX ) == 1 )
        {
            if( sidebar_size == 0 )
            {
                sidebar_size = PlayResX / 8;
            }
            fprintf( fpout, "PlayResX: %d\n", PlayResX - sidebar_size * 2 );
        }
        else if( sscanf( buf, "Dialogue: %d,%01d:%02d:%02d.%02d,%d:%02d:%02d.%02d,",
                    &Layer, &sh, &sm, &ss, &sms, &eh, &em, &es, &ems ) == 9 )
        {
            ass_str = buf;
            for( int i = 0; i < 3; i++ )
            {
                ass_str = strchr( ass_str, ',' );
                ass_str++;
            }
            int start = ((sh * 60 + sm) * 60 + ss) * 1000 + sms * 10;       // Floor
            int end   = ((eh * 60 + em) * 60 + es) * 1000 + ems * 10;       // Floor

            start = frame2time( get_cut_frame_number( time2frame( start ), +1 ) ) + ts_read_start_delay;
//            end   = frame2time( get_cut_frame_number( time2frame( end   ), -1 ) ) + ts_read_start_delay;
            if( start >= 0 )
                end = frame2time( get_cut_frame_number( time2frame( end ), -1 ) ) + ts_read_start_delay;
            else
                end = start;
            // delay        // memo: no check, MAX over.
            //  ※ 先にdelayを利かすと↑の判定でマイナス値をとる際にNG...
            //      → 通常用途外とし、現状は対処しない..
            start = ( (start + delay_time) >= 0 ) ? (start + delay_time) : 0;
            end   = ( (end   + delay_time) >= 0 ) ? (end   + delay_time) : 0;

            if( start >= end )
                continue;

            start += 5;             // Round Up/Down
            sms = start % 1000 / 10;
            start /= 1000;
            ss = start % 60;
            start /= 60;
            sm = start % 60;
            start /= 60;
            sh = start % 60;

            end += 5;               // Round Up/Down
            ems = end % 1000 / 10;
            end /= 1000;
            es = end % 60;
            end /= 60;
            em = end % 60;
            end /= 60;
            eh = end % 60;

            fprintf( fpout, "Dialogue: %d,%01d:%02d:%02d.%02d,%d:%02d:%02d.%02d,",
                    Layer,
                    sh, sm, ss, sms, eh, em, es, ems );
            if( side_cut || shiftX || shiftY )
            {
                int posX, posY;
                ass_str_pos = buf;
                for( int i = 0; i < 9; i++ )
                {
                    ass_str_pos = strchr( ass_str_pos, ',' );
                    ass_str_pos++;
                }
                if( sscanf( ass_str_pos, "{\\pos(%d,%d)", &posX, &posY ) == 2 )
                {
                    for( char *c = ass_str; c != ass_str_pos; c++ )
                        fputc( (int)*c, fpout );
                    ass_str_pos = strchr( ass_str_pos, ',' );
                    for( ; *ass_str_pos != ')'; ass_str_pos++ );
                    ass_str_pos++;
                    fprintf( fpout, "{\\pos(%d,%d)%s", posX - sidebar_size + shiftX, posY + shiftY, ass_str_pos );
                    continue;
                }
            }
            fprintf( fpout, "%s", ass_str );
        }
        else
            fputs( buf, fpout );
    }
    fclose( fpout );
    fclose( fpin );
}

void cut_srt( const char *file_in, const char *file_out )
{
    char fin_path[_MAX_PATH], fout_path[_MAX_PATH];
    strcpy( fin_path, file_in );
    strcat( fin_path, ".srt" );
    strcpy( fout_path, file_out );
    strcat( fout_path, ".srt" );

    int index = 1;
    FILE *fpin = fopen( fin_path, "rt" );
    if( !fpin )
    {
        debug_print_err( stderr, "input srt open error: %s\n", file_in );
        return;
    }
    FILE *fpout = fopen( fout_path, "wt" );
    if( !fpout )
    {
        debug_print_err( stderr, "output srt open error: %s\n", file_out );
        fclose( fpin );
        return;
    }
    char buf[256];
    while( fgets( buf, sizeof( buf ), fpin ) )
    {
        char *p = buf;
        if( !strncmp( buf, "\xEF\xBB\xBF", 3 ) )            // skip UTF-8 BOM
        {
            fprintf( fpout, "\xEF\xBB\xBF" );               // write UTF-8 BOM
            p += 3;
        }
        int n;
        if( sscanf( p, "%d\n", &n ) != 1 )
            continue;
        if( !fgets( buf, sizeof( buf ), fpin ) )
            break;

        int sh, sm, ss, sms;
        int eh, em, es, ems;
        if( sscanf( buf, "%d:%02d:%02d,%03d --> %d:%02d:%02d,%03d\n",
                    &sh, &sm, &ss, &sms, &eh, &em, &es, &ems ) == 8 )
        {
            int start = ((sh * 60 + sm) * 60 + ss) * 1000 + sms;
            int end   = ((eh * 60 + em) * 60 + es) * 1000 + ems;

            start = frame2time( get_cut_frame_number( time2frame( start ), +1 ) ) + ts_read_start_delay;
//            end   = frame2time( get_cut_frame_number( time2frame( end   ), -1 ) ) + ts_read_start_delay;
            if( start >= 0 )
                end = frame2time( get_cut_frame_number( time2frame( end ), -1 ) ) + ts_read_start_delay;
            else
                end = start;
            // delay        // memo: no check, MAX over.
            //  ※ 先にdelayを利かすと↑の判定でマイナス値をとる際にNG...
            //      → 通常用途外とし、現状は対処しない..
            start = ( (start + delay_time) >= 0 ) ? (start + delay_time) : 0;
            end   = ( (end   + delay_time) >= 0 ) ? (end   + delay_time) : 0;

            if( start >= end )
            {
                while( fgets( buf, sizeof( buf ), fpin ) )
                {
                    if( buf[0] == '\n' )
                        break;
                }
                continue;
            }

            sms = start % 1000;
            start /= 1000;
            ss = start % 60;
            start /= 60;
            sm = start % 60;
            start /= 60;
            sh = start % 60;

            ems = end % 1000;
            end /= 1000;
            es = end % 60;
            end /= 60;
            em = end % 60;
            end /= 60;
            eh = end % 60;

            fprintf( fpout, "%d\n%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\n",
                    index++,
                    sh, sm, ss, sms, eh, em, es, ems );
            bool noTextSubtite = true;
            while( fgets( buf, sizeof( buf ), fpin ) )
            {
                fputs( buf, fpout );
                if( buf[0] == '\n' )
                    break;
                noTextSubtite = false;
            }
            if( noTextSubtite )
                fputc( '\n', fpout );
        } else {
            fputs(buf, fpout);
        }
    }
    fclose( fpout );
    fclose( fpin );
}


// Clac String --> Result String.
void calc_numstring( char* buf, const int buf_size )
{
    int i = buf_size;

    debug_print_low( stderr,"   calc: %s\t", buf );

    char str_buf[256], buf_L[256], buf_R[256], buf_t[256], buf_t2[256];
    int str_index = 0,  pahlen_index = -1;
    while( str_index < i )
    {
        if( buf[str_index] == '(' )                 // '('
        {
            pahlen_index = str_index;
        }
        else if( buf[str_index] == ')' )            // ')'
        {
            //debug_print_log( stderr,"\n %d :%d", pahlen_index, str_index );           // check

            // buf Replace (Prev)
            int l;
            for( l = 0; l < pahlen_index; l++ )
            {
                buf_L[l] = buf[l];
            }
            buf_L[pahlen_index] = '\0';

            int j = 0;
            for( ; pahlen_index + 1 + j < str_index; j++ )
            {
                str_buf[j] = buf[pahlen_index+1+j];
            }
            str_buf[j] = '\0';
            //debug_print_log( stderr,"\ns_buf:%s  ,%d,%d\n",str_buf,str_index, j );    // check

            int str_num = str_index - 1 - pahlen_index;
            //debug_print_log( stderr,"\ns_buf:%s  num:%d\n",str_buf, str_num );        // check

            // calc
            int calc_num = 0;  bool calc_exctute = false;
            int k = 0, n = 0;
            if( str_buf[0] == '-' )
            {
                if( str_buf[1] == '-' )
                {
                    buf_t2[0] = '+';
                    k++;
                }
                else
                {
                    buf_t2[0] = '-';
                }
                k++;  n++;
            }
            for( ; k < str_num; k++, n++ )
            {
                if( str_buf[k] == '-'
                 || str_buf[k] == '+'
                 || str_buf[k] == '*'
                 || str_buf[k] == '/'
                 || str_buf[k] == '%' )
                {
                    buf_t2[n] = '\0';

                    int m = 0;
                    for( m = 0; k+1+m < str_num; m++ )
                         buf_t[m] = str_buf[k+1+m];
                    buf_t[m] = '\0';
                    int a = atoi( buf_t2 );
                    int b = atoi( buf_t );
                    //debug_print_log( stderr,"\n\tcalc:\t%d , %d\tb:%s %c %s\n",a,b,buf_t2,str_buf[k],buf_t );
                    if( str_buf[k] == '-')
                        calc_num = a - b;
                    else if( str_buf[k] == '+')
                        calc_num = a + b;
                    else if( str_buf[k] == '*')
                        calc_num = a * b;
                    else if( str_buf[k] == '/')
                        calc_num = a / b;
                    else if( str_buf[k] == '%')
                        calc_num = a % b;
                    calc_exctute = true;
                    //k += m;
                    break;
                }
                else if( '0' <= str_buf[k] && str_buf[k] <= '9' )
                    buf_t2[n] = str_buf[k];
            }

            // buf Replace
            strcpy( buf_t, buf_L );
            if( calc_exctute )
            {
                _itoa( calc_num, buf_R, 10 );
                strcat( buf_t, buf_R );
            }
            else
            {
                buf_t2[n] = '\0';
                strcat( buf_t, buf_t2 );
            }
            //debug_print_log( stderr," L:%s\t\tR:%s\t\t",buf_L, buf_R );       // check
            for( l = 0; str_index+1+l < i; l++ )
                buf_R[l] = buf[str_index+1+l];
            buf_R[l] = '\0';
            strcat( buf_t, buf_R );
            //debug_print_log( stderr," R:%s\t\tbuf:%s\n",buf_R, buf_t );       // check

            strcpy( buf, buf_t );
            str_index = 0;
            i = strlen( buf );
            //debug_print_log( stderr,"\n\t\tbuf:%s\n",buf );                   // check
            continue;
        }
        str_index++;
    }

    // last calc
    debug_print_low( stderr,"   last c: %s\t", buf );

    int calc_num = 0;  bool calc_exctute = false;
    int k = 0, n = 0;
    if( buf[0] == '-' )
    {
        if( buf[1] == '-' )
        {
            buf_t2[0] = '+';
            k++;
        }
        else
        {
            buf_t2[0] = '-';
        }
        k++;  n++;
    }
    for( ; k < i; k++, n++ )
    {
        if( buf[k]=='-'
         || buf[k]=='+'
         || buf[k]=='*'
         || buf[k]=='/'
         || buf[k]=='%' )
        {
            buf_t2[n] = '\0';

            int m = 0;
            for( m = 0; k+1+m < i; m++ )
            {
                 buf_t[m] = buf[k+1+m];
            }
            buf_t[m] = '\0';
            int a = atoi( buf_t2 );
            int b = atoi( buf_t );
            //debug_print_log( stderr,"\n\tcalc:\t%d , %d\tb:%s %c %s\n",a,b,buf_t2,buf[k],buf_t );     // check
            if( buf[k]=='-' )
                calc_num = a - b;
            else if( buf[k]=='+' )
                calc_num = a + b;
            else if( buf[k]=='*' )
                calc_num = a * b;
            else if( buf[k]=='/' )
                calc_num = a / b;
            else if( buf[k]=='%' )
                calc_num = a % b;
            calc_exctute = true;
            //k += m;
            break;
        }
        else if( '0' <= buf[k] && buf[k] <= '9' )
            buf_t2[n] = buf[k];
    }
    if( calc_exctute )
        _itoa( calc_num, buf, 10 );
    else
    {
        buf_t2[n] = '\0';
        strcpy( buf, buf_t2 );
    }

    debug_print_low( stderr,"\tresult:%s\n", buf );
}



//------------------------------------------------------------------------------
// Parse - DGIndex Project File
//------------------------------------------------------------------------------

int parse_d2v_info( const char *file_in, int &start_offset, char* fin_path )
{
    int result = -1;

    FILE *fpin = fopen( file_in, "rt" );
    if( !fpin )
    {
        debug_print_err( stderr, "input d2v open error: %s\n", file_in );
        return result;
    }

    char buf[_MAX_PATH];
    char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], fext[_MAX_EXT];

    // [DGIndexProjectFile*]
    // [file num]
    // [filename 1]
    // --> check 3 line
    for( int i = 0; i < 3; i++ )
    {
        if( !fgets( buf, sizeof( buf ), fpin ) )
            goto d2v_parse_end;
    }
    _splitpath( file_in, drive, dir, fname, fext );
    _makepath( fin_path, drive, dir, NULL, NULL );
    _splitpath( buf, drive, dir, fname, fext );
    if( strcmp( drive, "" ) )
    {
        _makepath( fin_path, drive, dir, fname, NULL );
    }
    else
    {
        strcat( fin_path, dir );
        strcat( fin_path, fname );
    }
//    strcat( fin_path, ".ts" );

    int start, s_offset, end, e_offset;

    while( fgets( buf, sizeof( buf ), fpin ) )
    {
        if( sscanf( buf, "Location=%x,%x,%x,%x", 
                    &start, &s_offset, &end, &e_offset ) == 4 )
        {
            start_offset = (s_offset - start) * 2048;
            result = 0;
            break;
        }
    }

d2v_parse_end:
    fclose( fpin );
    return result;
}



//------------------------------------------------------------------------------
// Parse - MPEG2-TS
//------------------------------------------------------------------------------


typedef enum {
    I_FRAME = 1,
    P_FRAME = 2,
    B_FRAME = 3
} ePictureCodingType;

const char picture_type[] = {
    '-',
    'I',
    'P',
    'B',
    '-'
};


static const char* aspect_ratio[] = {
    "", "1:1", "4:3", "16:9", "2.21:1"
};
static const double frame_rate[] = {
    0,
    24.0*1000.0/1001.0,
    24,
    25,
    30.0*1000.0/1001.0,
    30,
    50,
    60.0*1000.0/1001.0,
    60,
    -1,
};


#define TS_PACKET_LENGTH        (188)
#define SECTION_BUFFER_LENGTH    (TS_PACKET_LENGTH * 2)

static int parse_ts_header( FILE* fp, int &packet_length, ushort &pid, byte &payload_unit_start_indicator, byte &adp_field_ctrl, byte &adp_field_length );
static int parse_PAT( FILE* fp, std::vector<int>& pid_list, int &packet_length, ushort &pmt_pid );
static int parse_PMT( FILE* fp, ushort pmt_pid, std::vector<int>& pid_list, int &packet_length, ushort &pcr_pid, ushort &video_pid, ushort &audio_pid );
static int parse_PMT_sub( FILE* fp, ushort pmt_pid, byte (&section)[SECTION_BUFFER_LENGTH], int section_idx, const ushort section_length );
static int parse_PMT_section( byte (&section)[SECTION_BUFFER_LENGTH], std::vector<int>& pid_list, const ushort pmt_section_length, ushort &video_pid, ushort &audio_pid );
static int64 parse_PCR( FILE* fp, byte adp_field_ctrl, byte adp_field_length, int &packet_length );
static void initialize_pes_check( void );
static int parse_video_PES( FILE* fp, byte payload_unit_start_indicator, byte adp_field_ctrl, byte adp_field_length, int &packet_length, int64 &start_video_pts, int64 &start_video_iframe_pts );
static int search_picture_header( byte buf[], const int buf_length, const int search_start, int64 pts, int64 &start_video_pts, int64 &start_video_iframe_pts );
static int parse_audio_PES( FILE* fp, byte payload_unit_start_indicator, byte adp_field_ctrl, byte adp_field_length, int &packet_length, int64 &start_audio_pts );
static int get_PTS_DTS( FILE *fp, byte start_code[4], int &packet_length, int64 &pts, int64 &dts );


static bool payload_unit_start_indicator_exist = false;
static bool check_start_uint_next = false;
static int64 start_uint_pts = 0LL;
static bool i_frame_exist = false;
static int ts_read_byte_count = 0;
static int gop_header_count = 0;
static byte check_buf[TS_PACKET_LENGTH + 4];


int parse_transport_stream( const char *file_in, int mode )
{
    if( mode < 0 )
    {
        debug_print_err( stderr, "function param err!!\n" );
        return -1;
    }
    char fin_path[_MAX_PATH];
    strcpy( fin_path, file_in );
    strcat( fin_path, ".ts");

    std::vector<int> pat_list;
    std::vector<int> pmt_list;
    byte buf[TS_PACKET_LENGTH];

    FILE *fp = fopen( fin_path, "rb" );

    if( !fp )
    {
        debug_print_err( stderr, "not exist TS file...\n" );
        return -1;
    }
    debug_print_low( stderr, "\n   ==== TS parse ====\n" );

    int packet_length;
    int64 pcr, start_pcr;
    int64 start_video_pts, start_video_iframe_pts, start_audio_pts;
    ushort pid, pmt_pid, pcr_pid, video_pid, audio_pid;


    // init
    pcr = start_pcr = 0LL;
    start_video_pts = start_video_iframe_pts = start_audio_pts = 0LL;
    pid = pcr_pid = video_pid = audio_pid = 0x0000;

    // set
    pmt_pid = select_pmt_pid;


    // STEP 1: PAT/PMT/PCR
    debug_print_low( stderr, "\n\t==== step1: PAT/PMT/PCR ====\n" );
    packet_length = TS_PACKET_LENGTH;
    ts_read_byte_count = 0;

    while( 1 )
    {
        // parse TS Header
        byte payload_unit_start_indicator = 0;
        byte adp_field_ctrl = 0;
        byte adp_field_length = 0;
        if( parse_ts_header( fp, packet_length, pid, payload_unit_start_indicator, adp_field_ctrl, adp_field_length) < 0 )
            break;

        // parse PAT
        if( pid == 0 && pmt_pid == 0 )
        {
            if( adp_field_ctrl > 1 )
            {
                if( adp_field_length )      // check: adp field length
                {
                    fseek( fp, adp_field_length, SEEK_CUR );
                    packet_length -= adp_field_length;
                }
            }
            else                            // stuffing byte '0'
            {
                fseek( fp, 1, SEEK_CUR );
                packet_length--;
            }
            pat_list.clear();
            debug_print_low( stderr, "\n\t Parse PAT\n" );
            parse_PAT( fp, pat_list, packet_length, pmt_pid );

            debug_print_low( stderr, "\t PAT listup:%d\n", pat_list.size() );
        }
        // parse PMT
        else if( pmt_pid && pid == pmt_pid )
        {
            if( adp_field_ctrl > 1 )
            {
                if( adp_field_length )          // check: adp field length
                {
                    fseek( fp, adp_field_length, SEEK_CUR );
                    packet_length -= adp_field_length;
                }
            }
            else                                // stuffing byte '0'
            {
                fseek( fp, 1, SEEK_CUR );
                packet_length--;
            }

            if( fread( buf, sizeof( byte ), 1, fp ) == 1 )      // get payload start
            {
                packet_length--;
                if( buf[0] == 0x02 )            // pid: pmt_pid, table_id: PMT
                {
                    pmt_list.clear();
                    debug_print_low( stderr, "\n\t Parse PMT  : 0x%04X\n", pid );
                    parse_PMT( fp, pmt_pid, pmt_list, packet_length, pcr_pid, video_pid, audio_pid );

                    debug_print_low( stderr, "\t PMT listup:%d\n", pmt_list.size() );
                }
            }

        }
        // parse PCR
        else if( pcr_pid && pid == pcr_pid )
        {

            if( adp_field_ctrl > 1 )            // check: adp field exist
            {
                debug_print_low( stderr, "\n\t Calc PCR   0x%04X\n", pid );
                pcr = parse_PCR( fp, adp_field_ctrl, adp_field_length, packet_length );

                if( pcr >= 0 )
                {
                    debug_print_low( stderr, "\t\t pcr: %lld [%lldms]\n", pcr, pcr/90 );
                    if( start_pcr == 0 )
                    {
                        start_pcr = pcr;
                        debug_print_low( stderr, "\t\t  set Start PCR\n" );
                    }

                    debug_print_low( stderr, "\n\t end PAT/PMT/PCR check\n" );
                    break;      // parse step1 end
                }
            }
            else
            {
                //fseek( fp, 1, SEEK_CUR );
                //packet_length--;
                //// none playload check, last skip.
            }

        }

        // next packet
        if( packet_length > 0 )
            fseek( fp, packet_length, SEEK_CUR );

        packet_length = TS_PACKET_LENGTH;
        ts_read_byte_count += TS_PACKET_LENGTH;
    }
    debug_print_low( stderr, "\n" );

    // Step 1 end: error check
    if( start_pcr == 0 || video_pid == 0 )
    {
        debug_print_err( stderr, "\not found PID!!  pcr_pid:0x%4X, video_pid:0x%4X, audio_pid:0x%4X \n", start_pcr, video_pid, audio_pid );
        return -1;
    }


    // STEP 2-3: check item
    int picture_coding_type_cnt = 0;
    int payload_unit_start_indicator_exist_video_cnt = 0;
    int payload_unit_start_indicator_exist_audio_cnt = 0;


    // STEP 2: Video PID - PTS
//if( video_pid )
//{
    debug_print_low( stderr, "\n\t==== step2: Video PID - PTS ====\n" );
    packet_length = TS_PACKET_LENGTH;
    ts_read_byte_count = 0;
    fseek( fp, 0, SEEK_SET );
//    fseek( fp, d2v_start_offset, SEEK_SET );

    // init
    initialize_pes_check();

    while( 1 )
    {
        // parse TS Header
        byte payload_unit_start_indicator = 0;
        byte adp_field_ctrl = 0;
        byte adp_field_length = 0;
        if( parse_ts_header( fp, packet_length, pid, payload_unit_start_indicator, adp_field_ctrl, adp_field_length ) < 0 )
            break;

        //if( video_pid && pid == video_pid )
        if( pid == video_pid )
        {
            if( payload_unit_start_indicator )
                debug_print_low( stderr, "\n\t Parse PES  : 0x%04X\n", pid );

            int picture_num = parse_video_PES( fp, payload_unit_start_indicator, adp_field_ctrl, adp_field_length, packet_length, start_video_pts, start_video_iframe_pts );
            if( picture_num < 0 )
            {
                if( !payload_unit_start_indicator_exist )
                    payload_unit_start_indicator_exist_video_cnt++;
            }
            else
                picture_coding_type_cnt += picture_num;

            if( start_video_pts /* && start_pcr */ )
            {
                debug_print_low( stderr, "\n\t end Video PTS check\n" );
                break;
            }

        }

        if( packet_length > 0 )
            fseek( fp, packet_length, SEEK_CUR );
        packet_length = TS_PACKET_LENGTH;
        ts_read_byte_count += TS_PACKET_LENGTH;
    }
    debug_print_low( stderr, "\n" );

//} // Video PID - PTS


    // STEP 3: Audio PID - PTS
if( audio_pid )
{
    debug_print_low( stderr, "\n\t==== step3: Audio PID - PTS ====\n" );
    packet_length = TS_PACKET_LENGTH;
    ts_read_byte_count = 0;
    fseek( fp, 0, SEEK_SET );

    // init
    initialize_pes_check();

    while( 1 )
    {
        // parse TS Header
        byte payload_unit_start_indicator = 0;
        byte adp_field_ctrl = 0;
        byte adp_field_length = 0;
        if( parse_ts_header( fp, packet_length, pid, payload_unit_start_indicator, adp_field_ctrl, adp_field_length) < 0 )
            break;

        //if( audio_pid && pid == video_pid )
        if( pid == audio_pid )
        {
            if( payload_unit_start_indicator )
                debug_print_low( stderr, "\n\t Parse PES  : 0x%04X\n", pid );

            int picture_num = parse_audio_PES( fp, payload_unit_start_indicator, adp_field_ctrl, adp_field_length, packet_length, start_audio_pts );
            if( picture_num < 0 )
            {
                if( !payload_unit_start_indicator_exist )
                    payload_unit_start_indicator_exist_audio_cnt++;
            }
            else
            {
                //
            }

            if( start_audio_pts /* && start_pcr */ )
            {
                debug_print_low( stderr, "\n\t end Audio PTS check\n" );
                break;
            }

        }

        if( packet_length > 0 )
            fseek( fp, packet_length, SEEK_CUR );
        packet_length = TS_PACKET_LENGTH;
        ts_read_byte_count += TS_PACKET_LENGTH;
    }
    debug_print_low( stderr, "\n" );
} // Audio PID - PTS


    fclose( fp );

    debug_print_low( stderr, "\n" );
    debug_print_low( stderr, "\t==== TS Parse info ====\n" );
    debug_print_low( stderr, "\t -- Start PCR: %lld [%lldms]\n", start_pcr, start_pcr / 90 );
//if( video_pid )
//{
    debug_print_low( stderr, "\t -- Start Video PTS: %lld [%lldms]\n", start_video_pts, start_video_pts / 90 );
    debug_print_low( stderr, "\t -- Start Video PTS I-Frame: %lld [%lldms]\n", start_video_iframe_pts, start_video_iframe_pts / 90 );
    debug_print_low( stderr, "\t -- check  Frame Count: %d\n", picture_coding_type_cnt );
    debug_print_low( stderr, "\t -- through Vpid Count: %d\n", payload_unit_start_indicator_exist_video_cnt );
//}
if( audio_pid )
{
    debug_print_low( stderr, "\t -- Start Audio PTS: %lld [%lldms]\n", start_audio_pts, start_audio_pts / 90 );
    debug_print_low( stderr, "\t -- through Apid Count: %d\n", payload_unit_start_indicator_exist_audio_cnt );
}


    int read_delay_fistframe = 0;
    int read_delay_gopiframe = 0;
    int read_delay_first_aac = 0;

    if( start_pcr && start_video_pts && start_video_iframe_pts )
    {
        int loop_cnt;
        int loop_check_value = 5 * 1000;

        loop_cnt = ( start_video_pts + loop_check_value < start_pcr ) ? 1 : 0;
        read_delay_fistframe = (start_pcr - start_video_pts - (0x1FFFFFFFFLL)*loop_cnt) / 90;

        loop_cnt = ( start_video_iframe_pts + loop_check_value < start_pcr ) ? 1 : 0;
        read_delay_gopiframe = (start_pcr - start_video_iframe_pts - (0x1FFFFFFFFLL)*loop_cnt) / 90;

        loop_cnt = ( start_audio_pts + loop_check_value < start_pcr ) ? 1 : 0;
        read_delay_first_aac = (start_pcr - start_audio_pts - (0x1FFFFFFFFLL)*loop_cnt) / 90;

        switch (mode)
        {
            case 0:
                ts_read_start_delay = read_delay_fistframe;
                break;
            case 1:
                ts_read_start_delay = read_delay_gopiframe;
                break;
            case 2:
                ts_read_start_delay = ( abs( read_delay_first_aac ) < abs( read_delay_fistframe ) ) ? read_delay_first_aac : read_delay_fistframe;
                break;
            default:
                break;
        }
    }
    debug_print_low( stderr, "\n" );
//    debug_print_log( stderr, "\tTS read delay : %d\n", ts_read_start_delay );
    debug_print_log( stderr, "\tTS read delay\n" );
    debug_print_log( stderr, "\t -- dgindex : %d msec\n", read_delay_fistframe );
    debug_print_log( stderr, "\t -- mpg2vfp : %d msec\n", read_delay_gopiframe );
    debug_print_log( stderr, "\t -- tmpgenc : %d msec\n", ( abs( read_delay_first_aac ) < abs( read_delay_fistframe ) ) ? read_delay_first_aac : read_delay_fistframe );
    debug_print_log( stderr, "\n");

    return 1;
}


static int parse_ts_header( FILE* fp, int &packet_length, ushort &pid, byte &payload_unit_start_indicator, byte &adp_field_ctrl, byte &adp_field_length )
{
    byte buf[TS_PACKET_LENGTH];

    while( fread( buf, sizeof( byte ), 1, fp ) == 1 )
    {
        if( buf[0] != '\x47' )
        {
            ts_read_byte_count++;
            continue;
        }
        packet_length--;

        if( fread( buf, sizeof( byte ), 2, fp ) != 2 )
            continue;
        packet_length -= 2;

        pid = (ushort)(buf[0] & 0x1F) << 8 | buf[1];        // pid 13bit
        payload_unit_start_indicator = (buf[0] & 0x40);

        adp_field_ctrl = 0;
        if( fread( buf, sizeof( byte ), 1, fp ) == 1 )
        {
            packet_length--;

            adp_field_ctrl = (buf[0] >> 4) & 3;
            if( adp_field_ctrl > 1 )            // check: adp field exist
            {
                if( fread( buf, sizeof( byte ), 1 ,fp ) == 1 )
                {
                    packet_length--;
                    adp_field_length = (byte)buf[0];
                    //if( adp_field_length )          // check: adp field length
                    //{
                    //    fseek( fp, adp_field_length, SEEK_CUR );
                    //    packet_length -= adp_field_length;
                    //}
                    //debug_print_low( stderr, "\tcheck adp_f: %d,  %4u\n", adp_field_ctrl, adp_field_length );
                }
            }
            //else
            //{
            //    fseek( fp, 1, SEEK_CUR );
            //    packet_length--;
            //}
        }
        return 0;
    }

    return -1;
}
static int parse_PAT( FILE* fp, std::vector<int>& pid_list, int &packet_length, ushort &pmt_pid )
{
    byte buf[TS_PACKET_LENGTH];
    ushort pat_section_length = 0;


    if( fread( buf, sizeof( byte ), 1, fp ) != 1 )
        return -1;
    packet_length--;
    if( buf[0] != 0x00 )
        return -1;

    if( fread( buf, sizeof( byte ), 2, fp ) != 2 )
        return -1;
    packet_length -= 2;

    pat_section_length = (ushort)((buf[0] << 8 | buf[1]) & 0x0FFF);         // 12bit  section length
    debug_print_low( stderr, "\t\tpacket len:%5d", packet_length );
    debug_print_low( stderr, "    section len:%5u\n", pat_section_length );

    fseek( fp, 5, SEEK_CUR );               // PAT Head
    packet_length -= 5;
    pat_section_length -= 5;

    ushort pid_in_pat = 0;
    while( pat_section_length > 4 )         // 4 = CRC32
    {
        if( fread( buf, sizeof( byte ) , 4, fp ) != 4 )
            break;
        packet_length -= 4;
        pat_section_length -= 4;

        ushort program_id = (ushort)(buf[0] << 8 | buf[1]);
        pid_in_pat = (ushort)((buf[2] << 8 | buf[3]) & 0x1FFF);    // 13bit
        if( pid_in_pat == 0x1FFF )
            break;
        if( pid_in_pat == 0 )
            continue;
        debug_print_low( stderr, "\t\tprg_id:%6d,  pid:0x%04X,  left:%4d\n", program_id, pid_in_pat, pat_section_length );
        if( program_id && pmt_pid == 0 )
        {
            pmt_pid = pid_in_pat;    // first pmt
            //break;
        }
        pid_list.push_back( pid_in_pat );
    }

    // check CRC32
    if( packet_length >= 4 )
    {
        if( fread( buf, sizeof( byte ), 4, fp ) == 4 )
        {
            debug_print_low( stderr, "\t\tcrc32: %02X %02X %02X %02X\n", buf[0], buf[1], buf[2], buf[3] );
            packet_length -= 4;
        }
    }

    return 0;
} // parse_PAT()

static int parse_PMT( FILE* fp, ushort pmt_pid, std::vector<int>& pid_list, int &packet_length, ushort &pcr_pid, ushort &video_pid, ushort &audio_pid )
{
    byte buf[TS_PACKET_LENGTH];
    ushort pmt_section_length = 0;


    if( fread( buf, sizeof( byte ), 2, fp) != 2 )
        return -1;
    packet_length -= 2;

    pmt_section_length = (ushort)((buf[0] << 8 | buf[1]) & 0x0FFF);         // 12bit  section length
    debug_print_low( stderr, "\t\tpacket len:%5d", packet_length );
    debug_print_low( stderr, "    section len:%5u\n", pmt_section_length );

    fseek( fp, 5, SEEK_CUR );
    packet_length -= 5;
    pmt_section_length -= 5;

    ushort pmt_program_info_length = 0;
    if( fread( buf, sizeof( byte ), 2, fp ) == 2 )
    {
        pcr_pid = (ushort)((buf[0] << 8 | buf[1]) & 0x1FFF);        // 13bit
        debug_print_low( stderr, "\t\tPCR: 0x%04X\n", pcr_pid );
        packet_length -= 2;
        pmt_section_length -= 2;
    }
    if( fread( buf, sizeof( byte ), 2, fp ) == 2 )
    {
        pmt_program_info_length = (ushort)((buf[0] << 8 | buf[1]) & 0x0FFF);    // 12bit (1&2bit '00')
        packet_length -= 2;
        pmt_section_length -= 2;
    }

    fseek( fp, pmt_program_info_length, SEEK_CUR );
    packet_length -= pmt_program_info_length;
    pmt_section_length -= pmt_program_info_length;
    debug_print_low( stderr, "\t\tprg_info_len:%5u\n", pmt_program_info_length );


    byte pmt_section[SECTION_BUFFER_LENGTH];
    int section_idx = 0;

    fread( pmt_section, sizeof( byte ), packet_length, fp );

    if( packet_length < pmt_section_length )
    {
        // search, next packet.
        section_idx = packet_length;
        parse_PMT_sub( fp, pmt_pid, pmt_section, section_idx, pmt_section_length );
    }
    parse_PMT_section( pmt_section, pid_list, pmt_section_length, video_pid, audio_pid );

    packet_length = 0;

    return 0;
} // parse_PMT()


static int parse_PMT_sub( FILE* fp, ushort pmt_pid, byte (&section)[SECTION_BUFFER_LENGTH], int section_idx, const ushort section_length )
{
    fpos_t fpos;
    fgetpos( fp, &fpos );

//    byte buf[TS_PACKET_LENGTH];
    int packet_length = TS_PACKET_LENGTH;
    ushort pid = 0;
    byte payload_unit_start_indicator;        // not use.

    while( 1 )
    {
        // parse TS Header
        byte adp_field_ctrl = 0;
        byte adp_field_length = 0;
        if( parse_ts_header( fp, packet_length, pid, payload_unit_start_indicator, adp_field_ctrl, adp_field_length) < 0 )
            break;

        if( pmt_pid && pid == pmt_pid )
        {
            if( adp_field_ctrl > 1 )
            {
                if( adp_field_length )          // check: adp field length
                {
                    fseek( fp, adp_field_length, SEEK_CUR );
                    packet_length -= adp_field_length;
                }
            }
            //else                                // stuffing byte '0'
            //{
            //    fseek( fp, 1, SEEK_CUR );
            //    packet_length--;
            //}

#if 0
//            if( fread( buf, sizeof( byte ), 1, fp ) == 1 )        // get payload start
//            {
//                packet_length--;
//                if( buf[0] != 0x02 )            // pid: pmt_pid, table_id: athor
//                {
//                    debug_print_low( stderr, "\t   parse PMT table_id  : 0x%02X  , %d  , %d \n", buf[0], section_idx, section_length );
//
//                    fread( &(section[section_idx]), sizeof( byte ), section_length - section_idx, fp );
//                    break;
//                }
//            }
#else
            fread( &(section[section_idx]), sizeof( byte ), section_length - section_idx, fp );
            packet_length -= section_length - section_idx;
            break;
#endif    // 0

        }

    }

    fsetpos( fp, &fpos );
    return 0;
} // parse_PMT_sub()

static int parse_PMT_section( byte (&section)[SECTION_BUFFER_LENGTH], std::vector<int>& pid_list, const ushort pmt_section_length, ushort &video_pid, ushort &audio_pid )
{
    ushort pid_in_pmt = 0;
    ushort pmt_es_info_length;
    int idx = 0;

    while( idx < pmt_section_length - 9 )           // 4+5: 4 = CRC32
    {
        byte es_type = (byte)section[idx+0];
        pid_in_pmt = (ushort)((section[idx+1] << 8 | section[idx+2]) & 0x1FFF);    // 13bit
        pmt_es_info_length = (ushort)((section[idx+3] << 8 | section[idx+4]) & 0x0FFF);    // 12bit (1&2bit '00')

        pid_list.push_back( pid_in_pmt );
        idx += 5;
        idx += pmt_es_info_length;

        debug_print_low( stderr, "\t\t 0x%04X:", pid_in_pmt );
        debug_print_low( stderr, " %5u,  left:%5d,  es_type:0x%02X\n", pmt_es_info_length, pmt_section_length - idx, es_type );
        if( es_type == 0x02 )       // MPEG2
        {
            if( video_pid == 0 )        // first pid
                video_pid = pid_in_pmt;
        }
        else if( es_type == 0x0F )  // AAC
        {
            if( audio_pid == 0 )        // first pid
                audio_pid = pid_in_pmt;
        }
    }
    //debug_print_low( stderr, "\t\t left:%5d\n", pmt_section_length-idx );

    // check CRC32
    if( pmt_section_length - idx == 4 )
    {
        debug_print_low( stderr, "\t\tcrc32: %02X %02X %02X %02X\n", section[idx+0], section[idx+1], section[idx+2], section[idx+3] );
    }

    return 0;
} // parse_PMT_section()

static int64 parse_PCR( FILE* fp, byte adp_field_ctrl, byte adp_field_length, int &packet_length )
{
    int64 pcr = 0LL;
    byte buf[TS_PACKET_LENGTH];

//    debug_print_low( stderr, "\t Calc PCR   0x%04X\n", pid );

    //if( adp_field_length )              // check: adp field length
    if( adp_field_length >= 7 )         // check: adp field length
    {
        //debug_print_low( stderr, "\t\tcheck adp_f: %d,  %4u\n", adp_field_ctrl, adp_field_length );
        if( fread( buf, sizeof( byte ), 7, fp ) == 7 )
        {
            packet_length -= 7;

            //debug_print_low( stderr, "\t\t dis:%x, randm:%x, pri:%x\n", (buf[0]&0x80)>>7, (buf[0]&0x40)>>6, (buf[0]&0x20)>>5 );
            //debug_print_low( stderr, "\t\t 5flags: 0x%02x\n", buf[0]&0x1F );

            pcr = ( (int64) buf[1] << 25 |
                     (uint) buf[2] << 17 |
                     (uint) buf[3] <<  9 |
                     (uint) buf[4] <<  1 |
                     (uint) buf[5] / 128 );
            return pcr;
        }
        //fseek( fp, adp_field_length, SEEK_CUR );
        //packet_length -= adp_field_length;
    }

    return -1;
} // parse_PCR()

static void initialize_pes_check( void )
{
    payload_unit_start_indicator_exist = false;
    check_start_uint_next = false;
    start_uint_pts = 0LL;
    i_frame_exist = false;
    gop_header_count = -1;
}

static int parse_video_PES( FILE* fp, byte payload_unit_start_indicator, byte adp_field_ctrl, byte adp_field_length, int &packet_length, int64 &start_video_pts, int64 &start_video_iframe_pts )
{
//    byte buf[TS_PACKET_LENGTH];
    int picture_num = 0;
    int64 pts, dts;

    pts = dts = 0LL;

    if( payload_unit_start_indicator )
    {
        payload_unit_start_indicator_exist = true;

        if( adp_field_ctrl > 1 )        // check: adp field exist
        {
            if( adp_field_length )          // check: adp field length
            {
                fseek( fp, adp_field_length, SEEK_CUR );
                packet_length -= adp_field_length;
            }
            //debug_print_low( stderr, "\t\tcheck adp_f: %d,  %4u\n", adp_field_ctrl, adp_field_length );

        }
        else
        {
            //fseek(fp, 1, SEEK_CUR);
            //packet_length--;
            //// none stuffing byte "0"
        }

        // check PES packet
        byte start_code[4] = { 0x00, 0x00, 0x01, 0xE0 };
        if( get_PTS_DTS( fp, start_code, packet_length, pts, dts ) >= 0 )
        {
            if( (int)fread( check_buf, sizeof( byte ), packet_length, fp ) == packet_length )
            {
                if( check_buf[0] == 0x00
                 && check_buf[1] == 0x00
                 && check_buf[2] == 0x01
                 && check_buf[3] == 0xB3 )
                {
                    short horizontal_size = (check_buf[4] << 4) | (check_buf[5] >> 4);
                    short vertical_size = ((check_buf[5] & 0x0F) << 8) | (check_buf[6]);
                    byte aspect_ratio_code = check_buf[7] >> 4;
                    byte frame_rate_code = check_buf[7] & 0x0F;
                    debug_print_low( stderr, "\t\t -- Sequence Header --\n" );
                    debug_print_low( stderr, "\t\t frame_size: %dx%d\n", horizontal_size, vertical_size );
                    debug_print_low( stderr, "\t\t aspect_ratio: %s\n", aspect_ratio[aspect_ratio_code] );
                    debug_print_low( stderr, "\t\t frame_rate: %f\n", frame_rate[frame_rate_code] );
                    check_start_uint_next = true;
                    start_uint_pts = pts;

                    // Sequence Header --> Search GOP start code & Picture Header
                    int search_start = 4;
                    int picture_header_pos = search_picture_header( check_buf, packet_length, search_start, start_uint_pts, start_video_pts, start_video_iframe_pts );

                    if( search_start <= picture_header_pos && picture_header_pos < packet_length - 3 )
                    {
                        picture_num = 1;
                        //check_start_uint_next = false;
                        //start_uint_pts = 0LL;
                    }
                    else
                    {
                        memcpy( check_buf, &(check_buf[packet_length-3]), 3 );      // continue, next packet.
                    }

                }
                else if( check_buf[0] == 0x00
                      && check_buf[1] == 0x00
                      && check_buf[2] == 0x01
                      && check_buf[3] == 0x00 )
                {
                    ushort temporal_reference = ( (uint)(check_buf[4] << 2) | ((check_buf[5] & 0xC0) >> 6) );
                    byte picture_coding_type = (check_buf[5] & 0x38) >> 3;
                    debug_print_low( stderr, "\t\t -- Picture Header --\n" );
                    debug_print_low( stderr, "\t\t picture_coding_type: %d [%c], temporal_reference: %d\n", picture_coding_type, picture_type[picture_coding_type], temporal_reference );
                    picture_num = 1;

                    if( picture_coding_type == I_FRAME && start_video_iframe_pts == 0 )
                    {
                        start_video_iframe_pts = pts;
                        i_frame_exist = true;
                        debug_print_low( stderr, "\t\t  set Start Video PTS I-Frame\n" );
                    }
                    if( start_video_pts == 0 && temporal_reference == 0 && i_frame_exist )
                    {
                        if( check_gop_header_num < 0 && 0 < d2v_start_offset )
                        {
                            if( d2v_start_offset <= ts_read_byte_count )
                            {
                                start_video_pts = pts;
                                debug_print_low( stderr, "\t\t  set Start Video PTS\n" );
                            }
                        }
                        else if( check_gop_header_num <= gop_header_count )
                        {
                            start_video_pts = pts;
                            debug_print_low( stderr, "\t\t  set Start Video PTS\n" );
                        }
                    }

                }

                packet_length = 0;
            }
        }
        return picture_num;
    }
    else
    {
        // not payload_unit_start_indicator
        if( check_start_uint_next )
        {
            if( adp_field_ctrl > 1 )        // check: adp field exist
            {
                if( adp_field_length )          // check: adp field length
                {
                    fseek( fp, adp_field_length, SEEK_CUR );
                    packet_length -= adp_field_length;
                }
                //debug_print_low( stderr, "\t\tcheck adp_f: %d,  %4u\n", adp_field_ctrl, adp_field_length );
            }
            else
            {
                //fseek( fp, 1, SEEK_CUR );
                //packet_length--;
                //// none stuffing byte "0"
            }

            // check PES packet
            if( (int)fread( &(check_buf[3]), sizeof( byte ), packet_length, fp ) == packet_length )
            {
                // Search GOP start code & Picture Header
                int search_start = 0;
                int picture_header_pos = search_picture_header( check_buf, packet_length, search_start, start_uint_pts, start_video_pts, start_video_iframe_pts );

                if( search_start <= picture_header_pos && picture_header_pos < packet_length - 3 )
                {
                    picture_num = 1;
                    //check_start_uint_next = false;
                    //start_uint_pts = 0LL;
                }
                else
                    memcpy( check_buf, &(check_buf[packet_length-3]), 3 );      // continue, next packet.

                packet_length = 0;
            }
            return picture_num;
        }

    }

    return -1;
} // parse_video_PES()

static int search_picture_header( byte buf[], const int buf_length, const int search_start, int64 pts, int64 &start_video_pts, int64 &start_video_iframe_pts )
{
    int i = 0;

    if( search_start>buf_length - 3 )
        return -1;

    for( i = search_start; i < buf_length - 3; i++ )
    {
        if( buf[i+0] == 0x00
         && buf[i+1] == 0x00
         && buf[i+2] == 0x01
         && buf[i+3] == 0xB8 )
        {
            debug_print_low( stderr, "\t\t -- GOP start Code --\n" );

            gop_header_count++;

            // GOP start code  - 4byte shift
            if( i + 3 < buf_length - 3 )
                i += 3;
        }
        else if( buf[i+0] == 0x00
              && buf[i+1] == 0x00
              && buf[i+2] == 0x01
              && buf[i+3] == 0x00 )
        {
            ushort temporal_reference = ( (uint)(buf[i+4] << 2) | ((buf[i+5] & 0xC0) >> 6) );
            byte picture_coding_type = (buf[i+5] & 0x38) >> 3;
            debug_print_low( stderr, "\t\t -- Picture Header --\n" );
            debug_print_low( stderr, "\t\t picture_coding_type: %d [%c], temporal_reference: %d\n", picture_coding_type, picture_type[picture_coding_type], temporal_reference );
//            picture_num = 1;
            if( picture_coding_type == I_FRAME && start_video_iframe_pts == 0 )
            {
                start_video_iframe_pts = pts;
                i_frame_exist = true;
                debug_print_low( stderr, "\t\t  set Start Video PTS I-Frame\n" );
            }
            if( start_video_pts == 0 && temporal_reference == 0 && i_frame_exist )
            {
                if( check_gop_header_num < 0 && 0 < d2v_start_offset )
                {
                    if( d2v_start_offset <= ts_read_byte_count )
                    {
                        start_video_pts = pts;
                        debug_print_low( stderr, "\t\t  set Start Video PTS\n" );
                    }
                }
                else if( check_gop_header_num <= gop_header_count )
                {
                    start_video_pts = pts;
                    debug_print_low( stderr, "\t\t  set Start Video PTS\n" );
                }
            }
            check_start_uint_next = false;
            start_uint_pts = 0LL;
            break;
        }

    }

    return i;
}


static int parse_audio_PES( FILE* fp, byte payload_unit_start_indicator, byte adp_field_ctrl, byte adp_field_length, int &packet_length, int64 &start_audio_pts )
{
    byte buf[TS_PACKET_LENGTH];
//    int picture_num = 0;
    int64 pts, dts;

    pts = dts = 0LL;

    if( payload_unit_start_indicator )
    {
        payload_unit_start_indicator_exist = true;

        if( adp_field_ctrl > 1 )        // check: adp field exist
        {
            if( fread( buf, sizeof( byte ), 1, fp ) == 1 )
            {
                packet_length--;
                byte adp_field_length = (byte)buf[0];
                if( adp_field_length )      // check: adp field length
                {
                    fseek( fp, adp_field_length, SEEK_CUR );
                    packet_length -= adp_field_length;
                }
                //debug_print_low( stderr, "\t\tcheck adp_f: %d,  %4u\n", adp_field_ctrl, adp_field_length );
            }
        }
        else
        {
            //fseek( fp, 1, SEEK_CUR );
            //packet_length--;
            //// none stuffing byte "0"
        }

        // check PES packet
        byte start_code[4] = { 0x00, 0x00, 0x01, 0xC0 };
        if( get_PTS_DTS( fp, start_code, packet_length, pts, dts) >= 0 )
        {
            if( (int)fread( buf, sizeof( byte ), packet_length, fp ) == packet_length )
            {
                check_start_uint_next = true;
                for( int i = 0; i < packet_length - 2; i++ )
                {
                    if(  buf[i+0]         == 0xFF       // 0xFFF: sync word
                     && (buf[i+1] & 0xF8) == 0xF8       // 0x08: MPEG2-AAC
                     && (buf[i+2] & 0xC0) == 0x40 )     // 0x40: LC
                    // && (buf[i+2] & 0xFC) == 0x4C )     // 0x4C: LC, 48KHz
                    {
                        debug_print_low( stderr, "\t\t -- ADTS-AAC Header --\n" );
                        start_audio_pts = pts;
                        debug_print_low( stderr, "\t\t  set Start Audio PTS\n" );
                        check_start_uint_next = false;
                        break;
                    }
                }
                if( check_start_uint_next )
                {
                    check_buf[0] = buf[packet_length-2];
                    check_buf[1] = buf[packet_length-1];
                    start_uint_pts = pts;
                }
                packet_length = 0;
            }
        }
        return 0;
    }
    else
    {
        // not payload_unit_start_indicator
        if( check_start_uint_next )
        {
            if( adp_field_ctrl > 1 )            // check: adp field exist
            {
                if( fread( buf, sizeof( byte ), 1, fp ) == 1 )
                {
                    packet_length--;
                    byte adp_field_length = (byte)buf[0];
                    if( adp_field_length )         // check: adp field length
                    {
                        fseek( fp, adp_field_length, SEEK_CUR );
                        packet_length -= adp_field_length;
                    }
                    //debug_print_low( stderr, "\t\tcheck adp_f: %d,  %4u\n", adp_field_ctrl, adp_field_length );
                }
            }
            else
            {
                //fseek( fp, 1, SEEK_CUR );
                //packet_length--;
                //// none stuffing byte "0"
            }

            // check PES packet
            if( (int)fread( buf, sizeof( byte ), packet_length, fp ) == packet_length )
            {
                for( int i = 0; i < packet_length - 2; i++ )
                {
                    check_buf[2] = buf[i];
                    if(   check_buf[0]       == 0xFF        // 0xFFF: sync word
                      && (check_buf[1]&0xF8) == 0xF8        // 0x08: MPEG2-AAC
                      && (check_buf[2]&0xC0) == 0x40 )      // 0x40: LC
                     // && (check_buf[2]&0xFC) == 0x4C )      // 0x4C: LC, 48KHz
                    {
                        debug_print_low( stderr, "\t\t -- ADTS-AAC Header --\n" );
                        start_audio_pts = start_uint_pts;
                        debug_print_low( stderr, "\t\t  set Start Audio PTS\n" );
                        check_start_uint_next = false;
                        start_uint_pts = 0LL;
                        break;
                    }
                    check_buf[0] = check_buf[1];
                    check_buf[1] = check_buf[2];
                }
                packet_length = 0;
            }
            return 0;
        }

    }

    return -1;
} // parse_audio_PES()

static int get_PTS_DTS( FILE *fp, byte start_code[4], int &packet_length, int64 &pts, int64 &dts )
{
    byte buf[TS_PACKET_LENGTH];

    byte pes_packet_start_code[4];
    //byte steam_id;
    ushort pes_packet_length;
    byte mpeg_type;
    byte pes_header_length = 0;
    byte pts_dts_flag = 0;

    if( fread( pes_packet_start_code, sizeof( byte ), 4, fp ) == 4 )
    {
        packet_length -= 4;

        while( packet_length > 6 )
        {
            if( pes_packet_start_code[0] == start_code[0]
             && pes_packet_start_code[1] == start_code[1]
             && pes_packet_start_code[2] == start_code[2]
             && pes_packet_start_code[3] == start_code[3] )
            {
                if( fread( buf, sizeof( byte ), 5, fp ) == 5 )
                {
                    packet_length -= 5;

                    //steam_id = pes_packet_start_code[3];
                    pes_packet_length = (ushort)(buf[0] << 8 | buf[1]);
                    mpeg_type = (buf[2] & 0xC0) >> 6;
                    pts_dts_flag = (buf[3] & 0xC0) >> 6;
                    pes_header_length = buf[4];

                    debug_print_low( stderr, "\t\t check:  packet_length:%06d,  mpeg:%d,  header_length:%d\n", pes_packet_length, mpeg_type, pes_header_length );
                }
                break;
            }

            if( fread( buf, sizeof( byte ), 1, fp ) == 1 )
            {
                packet_length--;
                pes_packet_start_code[0] = pes_packet_start_code[1];
                pes_packet_start_code[1] = pes_packet_start_code[2];
                pes_packet_start_code[2] = pes_packet_start_code[3];
                pes_packet_start_code[3] = buf[0];
            }
            else
                break;        // error!!
        }
    }

    if( pes_header_length > 0 )
    {
        int read_size = 0;
        if( pes_header_length >= 10 )
            read_size = 10;
        else if( pes_header_length >= 5 )
            read_size = 5;

        if( pts_dts_flag && read_size
         && (int)fread( buf, sizeof( byte ), read_size, fp ) == read_size )
        {
            packet_length -= read_size;
            pes_header_length -= read_size;

            debug_print_low( stderr, "\t\t flag: %d", pts_dts_flag );
            if( (pts_dts_flag & 0x2) == 0x02 )
            {
                pts = (buf[0] & 0x0E) >> 1;
                pts <<= 15;
                pts |= ( (uint)((buf[1] << 8 | buf[2]) & 0xFFFE) >> 1);
                pts <<= 15;
                pts |= ( (uint)((buf[3] << 8 | buf[4]) & 0xFFFE) >> 1);
                debug_print_low( stderr, ",  pts: %lld [%lldms]", pts, pts / 90 );
            }
            if( (pts_dts_flag & 0x3) == 0x03 )
            {
                dts = (buf[5] & 0x0E) >> 1;
                dts <<= 15;
                dts |= ( (uint)((buf[6] << 8 | buf[7]) & 0xFFFE) >> 1);
                dts <<= 15;
                dts |= ( (uint)((buf[8] << 8 | buf[9]) & 0xFFFE) >> 1);
                debug_print_low( stderr, ",  dts: %lld [%lldms]", dts, dts / 90 );
            }
            debug_print_low( stderr, "\n" );
        }
        fseek( fp, pes_header_length, SEEK_CUR );
        packet_length -= pes_header_length;

        return 0;
    }

    return -1;
} // get_PTS_DTS()
