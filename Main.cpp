#include <Windows.h>
#include <process.h>
#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include <conio.h>

//関数プロトタイプ宣言！
unsigned __stdcall LoadThreadFunc(void *p);//スレッドで実行する読み込み関数
void LoadA(const std::string path, const std::string device);//読み込み関数
void LoadB(const std::string path, const std::string device);//読み込み関数２
void RegisterLoadFuncs();//読み込み関数を登録

//グローバル変数！
std::vector<std::function<void(std::string device)>> g_load_funcs;//読み込み関数を格納するための変数
unsigned int g_progress_num;//進捗具合を格納する変数
HANDLE g_progress_num_mutex;//排他用Mutexオブジェクト

int main()
{
	RegisterLoadFuncs();//読み込み関数を登録する（LoadThreadFuncスレッドの開始より前に実行してやる必要がある）

	//排他用Mutexオブジェクトの生成。進捗を表すg_progress_numにプライマリスレッドとLoadThreadFuncスレッドで同時にアクセスされないようにするため
	g_progress_num_mutex = CreateMutex(NULL, FALSE, NULL);

	unsigned load_thread_id;//読み込みスレッドのID
	std::string str("スレッドに渡した引数です");//スレッドに確認用として渡す文字列
	//スレッドの作成
	HANDLE load_thread_handle = reinterpret_cast<HANDLE>(//戻り値をHANDLE型にキャストしてやる必要がある
		_beginthreadex(NULL, 0,
		LoadThreadFunc,//実行するスレッド関数
		&str,//スレッド関数に渡す引き数、型はvoid*に暗黙的にキャストされます（使わないならNULLでおｋ）
		CREATE_SUSPENDED,//スレッド作成に関する制御フラグを指定します(0で即実行)
		&load_thread_id)//スレッドを識別するIDが入る
		);
	
	
	ResumeThread(load_thread_handle);//スレッドの起動

	while(1){//LoadThreadFuncスレッドが終わるまで無限ループ！
		DWORD end_load_thread_flag;//LoadThreadFuncスレッドが生きているかどうかを判定
		GetExitCodeThread(load_thread_handle, &end_load_thread_flag);//スレッド生存確認
		if (end_load_thread_flag != STILL_ACTIVE){//スレッドが終了していたら
			CloseHandle(load_thread_handle);//スレッドを削除
			break;//ループを抜ける
		}

		WaitForSingleObject(g_progress_num_mutex, INFINITE);//アクセス権の獲得
		std::cout << g_progress_num << "%" << std::endl;//進捗具合を表示
		ReleaseMutex(g_progress_num_mutex);//アクセス権の解放

		Sleep(1);//休憩！
	}
	CloseHandle(g_progress_num_mutex);//排他用Mutexオブジェクトの削除


	//最後に一回確認！（LoadThreadFuncスレッドはすでに終了しているので競合する可能性がないので排他制御はしなくてもよい）
	std::cout << g_progress_num << "%" << std::endl;//進捗具合を表示

	_getch();//キー何か押されるまで待機
	return 0;
}

unsigned __stdcall LoadThreadFunc(void *p)
{
	std::string str = *(reinterpret_cast<std::string *>(p));//void *型の引数はreinterpret_castで再解釈してやる必要がある
	std::cout << str << std::endl;//引き数の文字列表示
	const unsigned int kFuncMaxNum = g_load_funcs.size();//登録した読み込み関数の数を取得
	unsigned int func_count = 0;//実行完了した読み込み関数の数を数える
	const std::string kDevice("デバイス");//引き数に渡すよう

	//イテレータを用いて登録された読み込み関数を１つずつ最初から最後まで実行していく
	for(std::vector<std::function<void(std::string)>>::iterator it = g_load_funcs.begin();
		it != g_load_funcs.end();
		it++){//ここで次の関数へ参照先が変わる

			(*it)(kDevice);//登録された関数を一つ実行
			func_count++;//実行された読み込み関数をカウント
			float temp_g_progress_num = (static_cast<float>(func_count) / kFuncMaxNum) * 100.0f;//進捗具合を％で計算
			WaitForSingleObject(g_progress_num_mutex, INFINITE);//アクセス権の獲得
			g_progress_num = static_cast<unsigned int>(temp_g_progress_num);//進捗具合を他でも使えるように変数に代入
			ReleaseMutex(g_progress_num_mutex);//アクセス権の解放
	}

	return S_OK;
}

void LoadA(const std::string path, const std::string device)
{
	Sleep(2000);//疑似読み込み時間稼ぎ
	std::cout << "LoadAで" << path << "を読み込みました　引数＜" << device << "＞" << std::endl;
}

void LoadB(const std::string path, const std::string device)
{
	Sleep(2000);//疑似読み込み時間稼ぎ
	std::cout << "LoadBで" << path << "を読み込みました　引数＜" << device << "＞" << std::endl;
}

void RegisterLoadFuncs()
{
	//この関数内で読み込みの関数を全て登録しておく

	g_load_funcs.push_back([](std::string device){
		LoadA("サランラップ", device);//まだここでは登録するだけで実行はされない、あとで一気に実行するよ！
	});

	g_load_funcs.push_back([](std::string device){
		LoadB("ケチャップ", device);
	});

	g_load_funcs.push_back([](std::string device){
		LoadA("カマボコ", device);
	});

	g_load_funcs.push_back([](std::string device){
		LoadB("カレーライス", device);
	});
}
