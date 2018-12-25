#include <stdio.h>	
#include <string.h>	
#include <unistd.h>	
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <locale.h>

/*	define		*/
#define PATH_MAX	4096
#define NAME_MAX	255
#define FILES_DIR_NAME	"/Files/"	// 出題ファイル格納ディレクトリ
#define FILES_MAX	256		// 取り扱う出題ファイル数の最大
#define STRINGS_MAX	1024		// 一つの文字列の最大
#define QUESTION_MAX	2000		// 一つのファイルの最大出題数
#define SUCCESS		0
#define ERROR		1
#define EDITOR		"vi "

/*	struct	*/
struct filelist_struct {		// 番号とファイル名の組み合わせ
	unsigned char file_number;
	char file_name[NAME_MAX];
};

struct answer_and_question {
	unsigned long	number;
	unsigned long	rand_key;
	char answer[STRINGS_MAX];
	char question[STRINGS_MAX];
};

/*	function	*/
void wait_user_input(WINDOW *win, const char *format, const void *variable_p) {

	delch();
	mvwprintw(win, 0, 0, ":");
	wrefresh(win);
	wscanw(win, format, variable_p);
};

unsigned int fp_read_and_split(FILE *fp, struct answer_and_question *answer_and_question_s) {

	unsigned char cnt = 0,
		      cnt1 = 0;

	unsigned int question_max = 0;

	for (cnt = 0; !feof(fp); cnt++) {
		answer_and_question_s[cnt].number = cnt + 1;
		for(cnt1 = 0 ;; cnt1++){
			answer_and_question_s[cnt].answer[cnt1] = getc(fp);
			if (answer_and_question_s[cnt].answer[cnt1] == '\t') {
				answer_and_question_s[cnt].answer[cnt1] = '\0';
				break;
			}
			if (feof(fp)) break; 
		}
		for (cnt1 = 0; answer_and_question_s[cnt].question[cnt1] != '\n'; cnt1++) {
			answer_and_question_s[cnt].question[cnt1] = getc(fp);
			if (answer_and_question_s[cnt].question[cnt1] == '\n') {
				answer_and_question_s[cnt].question[cnt1] = '\0';
				question_max++;
				break;
			}
			if (feof(fp)) break;
		}
	}
	fclose(fp);
	return question_max;
}


/*	Main method	*/
int main(int argc,char** argv)
{
	/*	variable	*/
	WINDOW *question_moniter_wnd,
	       *background_wnd,
	       *text_wnd,
	       *border_text_wnd,
	       *question_text_wnd,
	       *user_input_box_wnd,
	       *user_input_wnd;

	DIR *files_dir;						// Files ディレクトリ
	FILE *reading_fp;					// 出題ファイルを格納
	struct dirent *files_dp;				// ディレクトリのデータを扱う構造体
	struct filelist_struct filelist_s[FILES_MAX];		// 出題ファイル一覧の構造体
	struct answer_and_question answer_and_question_s[QUESTION_MAX], // 出題番号,解答,問題,の構造体
	       answer_and_question_tmp;				// sort用

	char files_dir_path[PATH_MAX], 				// 出題ファイルのパス
	     user_input_y_or_n,					// ユーザ入力の y か n を格納する
	     user_input_answer[STRINGS_MAX],			// ユーザの解答を格納
	     command_line_str[strlen(EDITOR) + NAME_MAX];	// vi起動用

	unsigned char cnt = 0,
		      cnt1 = 0,				// ループ制御用変数
		      number_of_files = 0,			// 出題ファイル数
		      user_input_num = 0;		// ユーザーの入力した数

	unsigned short number_of_start_question = 0,	// 開始時の出題の行番号
		       number_of_end_question = 0;	// 最後の出題の行番号

	unsigned int question_max = 0;			// 最大出題数 (あとで sizeof の割り算に変更)

	/*	method		*/
	strcat(command_line_str, EDITOR);
	if (NULL == getcwd(files_dir_path, PATH_MAX)) {
		printf("カレントディレクトリの取得に失敗しました。\n");
		exit(ERROR);
	}

	if (PATH_MAX < (strlen(files_dir_path) + strlen(FILES_DIR_NAME))) {
		printf("出題ファイルまでのパスの長さが PATE_MAX(4096) 以上です。");
		exit(ERROR);
	} else {
		strcat(files_dir_path, FILES_DIR_NAME);
	}

	for (;;) {

		if (!(files_dir = opendir(files_dir_path))) {
			printf("%s ディレクトリが存在しません。\n", files_dir_path);
			exit(ERROR);
		}

		// 出題ファイルが格納されているディレクトリまで移動(エラー処理書く)
		if ((-1) == chdir(files_dir_path)) {
			printf("%s ディレクトリへの移動に失敗しました。\n", files_dir_path);
			exit(ERROR);
		};

		for (files_dp = readdir(files_dir), cnt = 0; files_dp != NULL; files_dp = readdir(files_dir)){
			/*	. と .. は一覧に代入しない	*/
			if ((!strcmp(files_dp->d_name,".")) || (!strcmp(files_dp->d_name,".."))) {
				continue;
			}
			filelist_s[cnt].file_number = cnt + 1;
			strcpy(filelist_s[cnt].file_name, files_dp->d_name);
			cnt++;
		}

		closedir(files_dir);

		if (0 == cnt) {
			printf("出題用のファイルが存在しません。\n%s.\nに出題用のファイルを作成してください。\n", files_dir_path);
			exit(ERROR);
		};

		number_of_files = cnt;		// 出題ファイルの量を代入

		setlocale(LC_ALL, "");
		initscr();
		start_color();
		echo();

//		getmaxyx(stdscr, h, w);

		/* generates color pair */
		init_pair(1, COLOR_BLACK, COLOR_WHITE); 
		init_pair(2, COLOR_BLUE, COLOR_WHITE); 
		init_pair(3, COLOR_WHITE, COLOR_BLACK); 

		/* generates new window */
		background_wnd = newwin(20, 59, 0, 0);
		border_text_wnd = newwin(1, 8 , 1, 1);
		text_wnd = newwin(1, 49, 1, 9);
		question_moniter_wnd = newwin(11, 55, 2, 2);
		question_text_wnd = newwin(9, 53, 3, 3);
		user_input_box_wnd = newwin(6, 55, 13, 2);
		user_input_wnd = newwin(4, 53, 14, 3);

		/* setting for background_wnd */
		wbkgd(background_wnd, COLOR_PAIR(1));
		box(background_wnd, ACS_VLINE, ACS_HLINE);

		/* setting for border_text_wnd */
		wbkgd(border_text_wnd, COLOR_PAIR(2));
		wattron(border_text_wnd, A_BOLD);
		wprintw(border_text_wnd, "CMEMWORD");

		/* setting for text_wnd */
		wbkgd(text_wnd, COLOR_PAIR(1));
		wprintw(text_wnd, " - implementation by ncurses. GUI of memword.");

		/* setting for question_moniter_wnd */
		wbkgd(question_moniter_wnd, COLOR_PAIR(1));
		wprintw(question_moniter_wnd, "+------<moniter>--------------------------------------+");

		for (cnt = 1 ; cnt <= 9; cnt++) {
			wmove(question_moniter_wnd, cnt, 0);
			wprintw(question_moniter_wnd, "|");
			wmove(question_moniter_wnd, cnt, 54);
			wprintw(question_moniter_wnd, "|");
		}

		mvwaddstr(question_moniter_wnd, 10, 0, "+-----------------------------------------------------+");

		/* setting ror question_text_wnd */

		/* setting for user_input_box_wnd */
		wbkgd(user_input_box_wnd, COLOR_PAIR(3));
		box(user_input_box_wnd, ACS_VLINE, ACS_HLINE);
		wmove(user_input_box_wnd, 0, 6);
		wprintw(user_input_box_wnd, "<user input box>");

		/* setting for user_input_wnd */

		/* print */
		touchwin(background_wnd);
		touchwin(border_text_wnd);
		touchwin(text_wnd);
		touchwin(question_moniter_wnd);
		touchwin(question_text_wnd);
		touchwin(user_input_box_wnd);
		touchwin(user_input_wnd);
		wrefresh(background_wnd);
		wrefresh(border_text_wnd);
		wrefresh(text_wnd);
		wrefresh(question_moniter_wnd);
		wrefresh(question_text_wnd);
		wrefresh(user_input_box_wnd);
		wrefresh(user_input_wnd);

		mvwaddstr(question_text_wnd, 0, 0, "==== memword start menu ====");
		mvwaddstr(question_text_wnd, 1, 0, "数値を入力して下さい。");
                mvwaddstr(question_text_wnd, 2, 0, "[  0] プログラム終了。\n");
                mvwaddstr(question_text_wnd, 3, 0, "[  1] 暗記を始める。\n");
                mvwaddstr(question_text_wnd, 4, 0, "[  2] 出題ファイルを編集する。\n");
		wrefresh(question_text_wnd);

                wait_user_input(user_input_wnd, "%hd", &user_input_num);

		if (1 == user_input_num) {
			/*	出題ファイル選択ループ	*/
			for (;;) {
				wclear(question_text_wnd);

				/*	Select		*/
				mvwaddstr(question_text_wnd, 0, 0, "(Please select a file and enter anumerical value)");
				mvwaddstr(question_text_wnd, 1, 0, "出題ファイルを数値で入力してください。");
				mvwaddstr(question_text_wnd, 2, 0, "[  0] メインメニューに戻る。");

				/*	出題ファイル番号と出題ファイル名を出力	*/
				for (cnt = 0; cnt < number_of_files; cnt++) {
					mvwprintw(question_text_wnd, (cnt + 3), 0, "[%3d] %s", filelist_s[cnt].file_number, filelist_s[cnt].file_name);
				}
				wrefresh(question_text_wnd);

				wait_user_input(user_input_wnd, "%hd", &user_input_num);

				/*	入力エラーチェック	*/
				if (user_input_num > number_of_files) {
					mvwaddstr(user_input_wnd, 0, 0, "実際の問題の量以上の値か、負の値が入力されました。");
					mvwaddstr(user_input_wnd, 1, 0, "A number greater than the actual number of file was entered.");
				} else {
					break;
				}
			}
				
			endwin();

			if (0 == user_input_num) continue;

			if ((reading_fp = fopen(filelist_s[user_input_num - 1].file_name, "r")) == NULL) {
				endwin();
				printf("ファイルの読み込みに失敗しました。\n");
				printf("file open error.\n");
				exit(ERROR);
			}

		} else if (2 == user_input_num) {

			for(;;) {
				/*	Select		*/
				wclear(question_text_wnd);
				wprintw(question_text_wnd, "作業内容を数値で選択してください。");
				mvwaddstr(question_text_wnd, 1, 0, "[  0] メインメニューに戻る。\n");
				mvwaddstr(question_text_wnd, 2, 0, "[  1] 既存のファイルを編集する。\n");
				mvwaddstr(question_text_wnd, 3, 0, "[  2] 新規ファイルを作成する。\n");
				wrefresh(question_text_wnd);
				wait_user_input(user_input_wnd, "%hd", &user_input_num);

				if (1 == user_input_num) {

					for (;;) {
						/*	Select		*/
						wclear(question_text_wnd);
						wprintw(question_text_wnd, "Please select a file and enter anumerical value.");
						mvwaddstr(question_text_wnd, 1, 0, "編集するファイルを数値で入力してください。");
						/*	出題ファイル番号と出題ファイル名を出力	*/
						mvwaddstr(question_text_wnd, 2, 0, "[  0] メインメニューに戻る。");
						for (cnt = 0;cnt < number_of_files; cnt++) {
							mvwprintw(question_text_wnd, cnt + 3, 0, "[%3d] %s\n", filelist_s[cnt].file_number, filelist_s[cnt].file_name);
						}
						wrefresh(question_text_wnd);
						wait_user_input(user_input_wnd, "%hd", &user_input_num);
						/*	入力エラーチェック	*/
						if (user_input_num > number_of_files) {
							wclear(user_input_wnd);
							wprintw(user_input_wnd, "実際の問題の量以上の値か、負の値が入力されました。");
							mvwaddstr(user_input_wnd, 1, 0, "A number greater than the actual number of file was entered.");
							wrefresh(user_input_wnd);
						} else {
							break;
						}
					}

					if (0 == user_input_num) break;

					strcat(command_line_str, filelist_s[user_input_num - 1].file_name);

					if (-1 == system(command_line_str)) {
						printf("shell が利用可能な状態では無いです。\n");
					};

					strcpy(command_line_str, EDITOR);

					} else if (2 == user_input_num) {
						if (-1 == system(command_line_str)) {
							printf("shell が利用可能な状態では無いです。\n");
						};
					} else if (0 == user_input_num) {
						break;
					} else {
					wclear(user_input_wnd);
					wprintw(user_input_wnd, "1か2以外が入力されました。\n");
					wrefresh(user_input_wnd);
				}
			}
		continue;
		} else if (0 == user_input_num) {
			endwin();
			printf("bye.\n");
			exit(SUCCESS);
		} else {
			endwin();
			printf("1か2以外が入力されました。\n");
			exit(ERROR);
		}

		user_input_num = 0;

		for (;;) {
			/*	select		*/
			wclear(question_text_wnd);
			mvwaddstr(question_text_wnd, 0, 0, "出題の順番を数値で入力して下さい");
			mvwaddstr(question_text_wnd, 1, 0, "[  0] メインメニューに戻る。");
			mvwaddstr(question_text_wnd, 2, 0, "[  1] 一行目から順番に出題する。");
			mvwaddstr(question_text_wnd, 3, 0, "[  2] ランダムに出題する。");
			mvwaddstr(question_text_wnd, 4, 0, "[  3] 解答の文字数が少ない順に出題する。");
			mvwaddstr(question_text_wnd, 5, 0, "[  4] 解答の文字数が多い順に出題する。");
			wrefresh(question_text_wnd);
			wait_user_input(user_input_wnd, "%hu", &user_input_num);

			if (1 == user_input_num) {
				question_max = fp_read_and_split(reading_fp, answer_and_question_s);
				break;
			/*	random		*/
			} else if (2 == user_input_num) {
				srand((unsigned)time(NULL));
				/*	解答と、出題を抽出	*/
				for (cnt = 0; !feof(reading_fp); cnt++) {
					for (cnt1 = 0 ;; cnt1++) {
						answer_and_question_s[cnt].answer[cnt1] = getc(reading_fp);
						if (feof(reading_fp)) break; 
						if (answer_and_question_s[cnt].answer[cnt1] == '\t') {
							answer_and_question_s[cnt].answer[cnt1] = '\0';
							break;
						}
					}
					for (cnt1 = 0; answer_and_question_s[cnt].question[cnt1] != '\n'; cnt1++) {
						answer_and_question_s[cnt].question[cnt1] = getc(reading_fp);
						if (feof(reading_fp)) break; 
						if (answer_and_question_s[cnt].question[cnt1] == '\n') {
							answer_and_question_s[cnt].question[cnt1] = '\0';
							question_max++;			// 出題数を数える
							break;
						}
					}
					if (feof(reading_fp)) break; 
					answer_and_question_s[cnt].number = cnt + 1;
					answer_and_question_s[cnt].rand_key = rand();
				}
				fclose(reading_fp);

				for (cnt = 0; cnt < question_max; cnt++) {
					for (cnt1 = 1; (cnt + cnt1) < question_max; cnt1++) {
						if (answer_and_question_s[cnt].rand_key < answer_and_question_s[cnt + cnt1].rand_key) {
							answer_and_question_tmp = answer_and_question_s[cnt];
							answer_and_question_s[cnt] = answer_and_question_s[cnt + cnt1];
							answer_and_question_s[cnt + cnt1] = answer_and_question_tmp;
						}
					}
				}
				break;

			/*	In order of decreasing number of characters	*/
			} else if (3 == user_input_num) {
				question_max = fp_read_and_split(reading_fp, answer_and_question_s);
				for (cnt = 0; cnt < question_max; cnt++) {
					for (cnt1 = 1; (cnt + cnt1) < question_max; cnt1++) {
						if (strlen(answer_and_question_s[cnt].answer) > strlen(answer_and_question_s[cnt + cnt1].answer)) {
							answer_and_question_tmp = answer_and_question_s[cnt];
							answer_and_question_s[cnt] = answer_and_question_s[cnt + cnt1];
							answer_and_question_s[cnt + cnt1] = answer_and_question_tmp;
						}
					}
				}

				for (cnt = 0; cnt < question_max; cnt++) {
					for (cnt1 = 1; (cnt + cnt1) < question_max; cnt1++) {
						if ((strlen(answer_and_question_s[cnt].answer) == strlen(answer_and_question_s[cnt + cnt1].answer)) &&
						(answer_and_question_s[cnt].answer[0] > answer_and_question_s[cnt + cnt1].answer[0])) {
							answer_and_question_tmp = answer_and_question_s[cnt];
							answer_and_question_s[cnt] = answer_and_question_s[cnt + cnt1];
							answer_and_question_s[cnt + cnt1] = answer_and_question_tmp;
						}
					}
				}
				break;
			/*	In descending order of the number of characters		*/
			} else if (4 == user_input_num) {
				question_max = fp_read_and_split(reading_fp, answer_and_question_s);

				for (cnt = 0; cnt < question_max; cnt++) {
					for (cnt1 = 1; (cnt + cnt1) < question_max; cnt1++) {
						if (strlen(answer_and_question_s[cnt].answer) < strlen(answer_and_question_s[cnt + cnt1].answer)) {
							answer_and_question_tmp = answer_and_question_s[cnt];
							answer_and_question_s[cnt] = answer_and_question_s[cnt + cnt1];
							answer_and_question_s[cnt + cnt1] = answer_and_question_tmp;
						}
					}
				}

				for (cnt = 0; cnt < question_max; cnt++) {
					for (cnt1 = 1; (cnt + cnt1) < question_max; cnt1++) {
						if ((strlen(answer_and_question_s[cnt].answer) == strlen(answer_and_question_s[cnt + cnt1].answer)) &&
						(answer_and_question_s[cnt].answer[0] < answer_and_question_s[cnt + cnt1].answer[0])) {
							answer_and_question_tmp = answer_and_question_s[cnt];
							answer_and_question_s[cnt] = answer_and_question_s[cnt + cnt1];
							answer_and_question_s[cnt + cnt1] = answer_and_question_tmp;
						}
					}
				}
				break;

			} else if (0 == user_input_num) {
				break;
			} else {
				printf("表示されている番号以外が入力されました。\n");
			}
		}

		if (0 == user_input_num) continue;

		for (;;) {
			/*	Select		*/
			wclear(question_text_wnd);
			mvwprintw(question_text_wnd, 0, 0, "出題数：%d", question_max);
			mvwaddstr(question_text_wnd, 1, 0, "全問出題しますか?\n(y/n)");
			wrefresh(question_text_wnd);
			wait_user_input(user_input_wnd, "%c", &user_input_y_or_n);

			if ('y' == user_input_y_or_n) {
				number_of_start_question = 0;
				number_of_end_question = question_max;
				break;
			} else if ('n' == user_input_y_or_n) {

				if ((1 == user_input_num) || (3 == user_input_num) || (4 == user_input_num)) {

					for(;;){
						/*	Select		*/
						wclear(question_text_wnd);
						wprintw(question_text_wnd, "出題数：%d", question_max);
						mvwaddstr(question_text_wnd, 1, 0, "何問目から出題しますか?");
						mvwaddstr(question_text_wnd, 2, 0 ,"数値を入力して下さい。");
						wrefresh(question_text_wnd);
						wait_user_input(user_input_wnd, "%hd", &user_input_num);
						
						/*	入力エラーチェック	*/
						if (user_input_num > question_max) {
							wclear(user_input_wnd);
							wprintw(user_input_wnd, "実際の問題の量以上の値か、負の値が入力されました。");
							mvwaddstr(user_input_wnd, 1, 0, "A number greater than the actual number of file was entered.\n");
							wrefresh(user_input_wnd);
							wait_user_input(user_input_wnd, "%hd", &user_input_num);
						} else if (0 == user_input_num) {
							wclear(user_input_wnd);
							wprintw(user_input_wnd, "0問目は存在しません。1以上の数値を入力して下さい。");
							wrefresh(user_input_wnd);
							wait_user_input(user_input_wnd, "%hd", &user_input_num);
						} else {
							break;
						}
					}

					number_of_start_question = (user_input_num - 1);

					for (;;) {
						/*	Select		*/
						wclear(question_text_wnd);
						wprintw(question_text_wnd, "出題開始行：%d", user_input_num);
						mvwprintw(question_text_wnd, 1, 0, "出題数：%d", question_max);
						mvwaddstr(question_text_wnd, 2, 0, "何問目まで出題しますか?");
						mvwaddstr(question_text_wnd, 3, 0 ,"数値を入力して下さい。");
						wrefresh(question_text_wnd);
						wait_user_input(user_input_wnd, "%hd", &number_of_end_question);

						/*	入力エラーチェック	*/
						if (number_of_end_question > question_max) {
							wclear(user_input_wnd);
							wprintw(user_input_wnd, "実際の問題の量以上の値か、負の値が入力されました。");
							mvwaddstr(user_input_wnd, 1, 0, "A number greater than the actual number of file was entered.");
							wrefresh(user_input_wnd);
							wait_user_input(user_input_wnd, "%c", &user_input_num);
						} else if (number_of_start_question > number_of_end_question) {
							wclear(user_input_wnd);
							wprintw(user_input_wnd, "出題開始行番号より小さい数値が入力されました。");
							mvwaddstr(user_input_wnd, 1, 0, "A number smaller than thaline number at which the question is to be stated has been entered.");
							wrefresh(user_input_wnd);
							wait_user_input(user_input_wnd, "%c", &user_input_num);
						} else {
							break;
						}
					}
				/*	random		*/
				} else if (2 == user_input_num) {

					number_of_start_question = 0;

					for (;;) {
						/*	Select		*/
						wclear(question_text_wnd);
						wprintw(question_text_wnd, "全出題数：%d", question_max);
                                                mvwaddstr(question_text_wnd, 1, 0, "何問出題しますか?");
                                                mvwaddstr(question_text_wnd, 2, 0, "数値を入力して下さい\n");
						wrefresh(question_text_wnd);
						wait_user_input(user_input_wnd, "%hd", &number_of_end_question);
                                                /*      入力エラーチェック      */
                                                if (number_of_end_question > question_max) {
							wclear(user_input_wnd);
                                                        wprintw(user_input_wnd, "実際の問題の量以上の値か、負の値が入力されました。");
                                                        mvwaddstr(user_input_wnd, 1, 0, "A number greater than the actual number of file was entered.");
							wrefresh(user_input_wnd);
                                                } else if (number_of_start_question > number_of_end_question) {
							wclear(user_input_wnd);
                                                        wprintw(user_input_wnd, "出題開始行番号より小さい数値が入力されました。");
                                                        mvwaddstr(user_input_wnd, 1, 0, "A number smaller than thaline number at which the question is to be stated has been entered.");
							wrefresh(user_input_wnd);
                                                } else {
                                                        break;
                                                }
					}
				}
				break;
			} else {
				wclear(user_input_wnd);
				wprintw(user_input_wnd, "y(yes) か n(no) を入力して下さい。");
				wrefresh(user_input_wnd);
				wait_user_input(user_input_wnd, "%c", user_input_answer);
				
			}
		}
		/*	出題	*/
		for (cnt = number_of_start_question, cnt1 = 1; cnt < number_of_end_question; cnt++, cnt1++) {

			wclear(question_text_wnd);
			mvwprintw(question_text_wnd, 0, 0, "question : #%hhu\tline number : #%lu", cnt1, answer_and_question_s[cnt].number);
			mvwprintw(question_text_wnd, 1, 0, "Q: %s\n", answer_and_question_s[cnt].question);
			wrefresh(question_text_wnd);
			wait_user_input(user_input_wnd, "%[^\t\n]", user_input_answer);

			if (!strcmp(answer_and_question_s[cnt].answer, user_input_answer)) {
				wclear(user_input_wnd);
				mvwprintw(user_input_wnd, 2, 0, "correct!!");
				mvwprintw(user_input_wnd, 3, 0, "A: %s", answer_and_question_s[cnt].answer);
				wrefresh(user_input_wnd);
			} else {
				wclear(user_input_wnd);
				mvwprintw(user_input_wnd, 2, 0, "miss!!");
				mvwprintw(user_input_wnd, 3, 0, "A: %s\n\n", answer_and_question_s[cnt].answer);
				wrefresh(user_input_wnd);
				cnt1--;
				cnt--;
			}
		}

		for (;;) {

			wclear(question_text_wnd);
			wprintw(question_text_wnd, "出題が終わりました。プログラムを終了しますか?(y/n)\n");
			wrefresh(question_text_wnd);
			wait_user_input(user_input_wnd, "%c", &user_input_y_or_n);

			if ('y' == user_input_y_or_n) {
				break;
			} else if ('n' == user_input_y_or_n) {
				question_max = 0;
				break;
			} else {
				wclear(question_text_wnd);
				wprintw(question_text_wnd, "y(yes) か n(no) を入力して下さい。\n\n");
				wrefresh(question_text_wnd);
			}
		}
		if ('y' == user_input_y_or_n) break;
	}

	endwin();
	exit(SUCCESS);
};
