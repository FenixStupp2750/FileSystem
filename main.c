#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define MAX_BLOCK_FOR_FILE 3
#define shift_superBlock 1024 // сдвиг до супер блока
#define blockSize 2048 // размер одного блока
#define maxLength blockSize*3 // максимальный размер файла в байтах
// Структура супер блока
typedef struct SuperBlock
{
	uint32_t shift_tableINode; // смещение до блока с таблицей inode'ов
	uint32_t shift_inodeBitMap; // смещение до блока с битовой картой занятости inodо'в
	uint32_t shift_dataBlockBitMap; // смещение до блока с битовой картой занятости блоков данных
	uint32_t shift_nameTable; // смещение до блока с таблицей имен файлов данных в системе системе
	uint32_t shift_dataGroup; // смещение до первого блока данных
	uint32_t shift_freeDataBlock; // смещение до первого свободного блока
	uint32_t num_dataBlock; // общее количество блоков данных
	uint32_t num_inode; // общее количество inode'ов в
	uint32_t cnt_freeDataBlock; // количество свободных блоков данных
	uint32_t cnt_freeInode; // количество свободных inode'ов
}SuperBlock;

// Структура inode-а
typedef struct INode
{
	uint32_t id; // ID inode-а
	uint32_t uid; // ID пользователя, создавшего
	uint32_t size; // Размер файла в байтах
	time_t createTime; // Дата и время создания файла
	uint16_t dataBlockPos[MAX_BLOCK_FOR_FILE]; // Номера блоков данных, выделенных для хранения файла
}INode;

#define inodeNum blockSize/sizeof(INode) // Размер блока
#define nameLen sizeof(INode) // Максимальная длина имени файла
#define blockNum blockSize/sizeof(INode)*3 // количество блоков данных, доступных в файловой системе
// Структура файловой системы
typedef struct FileSystemLoc
{
	SuperBlock sb; // Супер блок
	INode inodeArr[inodeNum]; // Таблица inode-ов
	char inodeBitMap[blockSize]; // Битовая карта занятости
	char dataBlockBitMap[blockSize]; // Битовая карта занятости блоков
	char nameTable[inodeNum][nameLen]; // Таблица имен файлов
	char dataBlock[blockNum][blockSize]; // Блоки данных
}FileSystemLoc;

int fd = NULL; // Файловый дескриптор
char fsfolder[] = "/home/danlin/Operation_System/FileSystem/fs/fs.txt"; // Место файловой системы
int readSuperBlock(SuperBlock* sb); // Функция чтения супер блока с жеского диска
int readINode(INode* inode, int id); // Функция чтения inode-а по заданному id из таблицы
int readName(char* name, int id); // Функция чтения имени файла по заданному id inode - а из таблицы
int readBlock(char* buffer, uint32_t shift); // Функция чтения блока данных с жеского диска по заданному смещению
int writeSuperBlock(SuperBlock* sb); // Функция записи супер блока на жеский диск
int writeINode(INode* inode, int id); // Функция записи inode-а с заданным id в таблицу
int writeName(char* name, int id); // Функция записи имени файла в таблицу с заданным id inode - а
int writeBlock(char* buffer, uint32_t shift); // Функция записи блока данных на жеский диск по заданному смещению
int clearINode(int id); // Функция освобождения inode-а
int clearName(int id); // Функция удаления имени файла из таблицы
int findName(char* name); // Функция поиска имени файла в таблице
int findFreeINode(); // Функция поиска первого свободного inode - а в битовой карте занятости inode - ов
int findFreeDataBLock(); // Функция поиска первого свободного блока данных в битовой карте занятости блоков данных
int invertBitINode(int id); // Функция меняющая значение бита с заданным номером в битовой карте занятости inode - ов
int invertBitDataBlock(int num); // Функция меняющая значение бита с заданным номером в битовой карте занятости блоков данных
void printINode(INode* inode); // Функция выводящая содержимое inode - а в терминал
void printFS(int arg); // Функция выводящая содержимое файловой системы в различных видах
void printHelpMessage(); // Функция выводящая справочную информацию по работе с файловой системой
int makeFileSystem(); // Функция создающая область памяти под файловую систему на жестком диске
int openFileSystem(); // Функция загружающая файловую систему при запуске данной программы
int fs_clear(); // Функция форматирования файловой системы
int file_create(char* name); // Функция создания нового файла
int file_delete(char* name); // Функция удаления существующего файла
int file_read(char* name); // Функция вывода содержимого существующего файла в терминал
int file_write(char* name, char* str);// Функция добавления строки символов в конец существующего файла

// main-функция в которой реализован пользовательский интерфейс для работы с файловой системой
int main()
{
	int flag = 1; // флаг работы программы
	int ret = 0; // переменная для сохранения возвращаемых значений
	char buffer[2 * nameLen]; // символьный буффер для сохранения вводимых пользователем данных
	// открытие файловой системы
	if (openFileSystem() == -1)
	{
		// Если не удалось открыть, то выводим соответствующее сообщение
		printf("Не удалось открыть файловую систему\n");
		return -1; // Завершаем программу с ошибкой
	}

	printf("Файловая система \x1b[31mv1.0\x1b[0m\n");
	printf("Размер блока \x1b[31m%d\x1b[0m байт\n", blockSize);
	printf("Максимальное число файлов - \x1b[31m%d\x1b[0m\n", inodeNum);
	printf("Максимальный размер файла - \x1b[31m%d\x1b[0m байт\n", maxLength);
	printf("Максимальная длина имени файла - \x1b[31m%d\x1b[0m символов\n\n", nameLen);
	printf("Все доступные команды для работы с файловой системой:\n");
	printf(" ├ \x1b[1;36m/?\x1b[0m Вывод подробной справки по всем командам.\n");
	printf(" ├ \x1b[1;36mcreate\x1b[0m Создание нового файла.\n");
	printf(" ├ \x1b[1;36mrem\x1b[0m Удаление существующего файла.\n");
	printf(" ├ \x1b[1;36mecho\x1b[0m Добавить строку в конец файла\n");
	printf(" ├ \x1b[1;36mout\x1b[0m Вывод содержимого файла на экран\n");
	printf(" ├ \x1b[1;36mls\x1b[0m Вывод списка файлов, хранящихся в файловой системе.\n");
	printf(" ├ \x1b[1;36mcleanup\x1b[0m Форматировать файловую систему\n");
	printf(" └ \x1b[1;36mexit\x1b[0m Выход из программы\n");
	// Далее в цикле реализация интерфейса пользователя
	while (flag)
	{
		printf("→→→→→ ");
		scanf("%s", buffer); // Считываем введенную пользователем кманду
		// Если введена команда /?
		if (strcmp(buffer, "/?") == 0)
		{
			printHelpMessage(); // Выводим справочного сообщения
		}
		// Если введена команда ls
		else if (strcmp(buffer, "ls") == 0)
		{
			char c = getchar(); // Считываем следующий за командой символ
			// Если это символ переноса строки
			if (c == '\n')
			{
				printFS(0); // Выводим список всех хранимых файлов
			}
			// Если какой-то другой символ
			else
			{

				int i = 0;
				memset(buffer, 0, 2 * nameLen); // Обнуляем
				// Сохраняем символы пока не достигнем переноса
				while ((c = getchar()) != '\n' && i < 2 * nameLen)
				{
					buffer[i] = c;
					i++;
				}
				buffer[i] = 0;
				// Если удалось считать ключ -l
				if (strcmp(buffer, "-det") == 0)
					printFS(1); // Выводим список всех хранимых файлов с подробной информацией
				// Иначе ошибка ввода команды
				else
					printf("\nНет такого ключа для команды ls\n");
			}
		}
		// Если введена команда create
		else if (strcmp(buffer, "create") == 0)
		{
			char c = getchar(); // Считываем следующий за командой
			// Если это символ переноса строки
			if (c == '\n')
			{
				printf("\nНе введено имя файла.\n"); // Выводим

			}
			// Если какой-то другой символ
			else
			{

				int i = 0;
				memset(buffer, 0, 2 * nameLen); // Обнуляем
				// Сохраняем символы пока не достигнем переноса
				while ((c = getchar()) != '\n' && i < 2 * nameLen)
				{
					buffer[i] = c;
					i++;
				}
				buffer[i] = 0;
				ret = 0;
				ret = file_create(buffer); // Запускаем функцию создания файла, передаем в нее считанное имя
				// Проверяем возвращенное функцией значение
				switch (ret)
				{
				case 0: // Ошибка: в файловой системе достугнуто максимальное количествофайлов

					printf("\nДостигнуто максимальное число файлов в файловой системе\n");
					break;
				case -1: // Ошибка: введено имя большей длины, чем максимальная разрешенная длина
					printf("\nЗадано слишком длинное имя.\n");
					break;
				case -2: // Ошибка: введено имя которое уже	используется в файловой системе существует\n");

					printf("\nФайл с таким именем уже % s.\n", buffer);
					break;
				default: // Без ошибки: файл создан успешно
					printf("\nФайл успешно создан. Name = % s.\n", buffer);
					break;
				}
			}
		}
		// Если введена команда rem
		else if (strcmp(buffer, "rem") == 0)
		{
			char c = getchar(); // Считываем следующий за командой символ
			// Если это символ переноса строки
			if (c == '\n')
			{
				printf("\nНе введено имя файла.\n"); // Выводим сообщение об ошибке
			}
			// Если какой-то другой символ
			else
			{
				int i = 0;
				memset(buffer, 0, 2 * nameLen); // Обнуляем
				// Сохраняем символы пока не достигнем переноса
				while ((c = getchar()) != '\n' && i < 2 * nameLen)
				{
					buffer[i] = c;
					i++;
				}
				buffer[i] = 0;
				ret = 0;
				ret = file_delete(buffer); // Запускаем функцию удаления файла, передаем в нее считанное имя
				// Проверяем возвращенное функцией значение
				switch (ret)
				{
				case 0: // Ошибка: в файловой системе не существует файла с заданным именем

					printf("\nВведено имя несуществующего файла.\n");
					break;
				case -1: // Ошибка: введено имя большей длины, чем максимальная разрешенная длина
					printf("\nЗадано слишком длинное имя.\n");
					break;

				case -2: // Ошибка: в файловой системе не хранится ни одного файла
					printf("\nВ файловой системе нет файлов.\n");
					break;
				default: // Без ошибки: файл удален успешно
					printf("\nФайл %s успешно удален.\n", buffer);
					break;
				}
			}
		}
		// Если введена команда out
		else if (strcmp(buffer, "out") == 0)
		{
			char c = getchar(); // Считываем следующий за командой
			// Если это символ переноса строки
			if (c == '\n')
			{
				printf("\nНе введено имя файла.\n"); // Выводим сообщение об ошибке
			}
			// Если какой-то другой символ
			else
			{
				int i = 0;
				memset(buffer, 0, 2 * nameLen); // Обнуляем
				// Сохраняем символы пока не достигнем переноса
				while ((c = getchar()) != '\n' && i < 2 * nameLen)
				{
					buffer[i] = c;
					i++;
				}
				buffer[i] = 0;
				ret = 0;
				ret = file_read(buffer); // Запускаем функцию чтения файла, передаем в нее считанное имя
				// Проверяем возвращенное функцией значение
				switch (ret)
				{
				case 0: // Ошибка: в файловой системе не существует файла с заданным именем

					printf("\nВведено имя несуществующего файла.\n");
					break;
				case -1: // Ошибка: введено имя большей длины, чем максимальная разрешенная длина
					printf("\nЗадано слишком длинное имя.\n");
					break;


				case -2: // Ошибка: в файловой системе не хранится ни одного файла
					printf("\nВ файловой системе нет файлов.\n");
					break;
				default: // Без ошибки: файл прочитан успешно
					printf("\nСодержимое файла %d успешно выведено.\n", buffer);
					break;
				}
			}
		}
		// Если введена команда echo
		else if (strcmp(buffer, "echo") == 0)
		{
			char c = getchar(); // Считываем следующий за командой
			// Если это символ переноса строки
			if (c == '\n')
			{
				printf("\nНе введено имя файла.\n"); // Выводим
			}

			// Если какой-то другой символ
			else
			{
				int i = 0;
				char str[maxLength]; // Создаем массив для ввода строки
				memset(buffer, 0, 2 * nameLen); // Обнуляем
				ret = 0;
				// Сохраняем символы пока не достигнем пробела
				while ((c = getchar()) != ' ' && i < 2 * nameLen)
				{
					if (c == '\n')
					{
						ret = -4;
						break;
					}
					buffer[i] = c;
					i++;
				}
				buffer[i] = 0;
				if (ret != -4)
				{
					c = getchar(); // Считываем предположительно открывающую кавычку
					// Если считалась двойная кавычка
					if (c == '\"')
					{
						// В цикле посимвольно сохраняем введенное сообщение пока не закончится место в массиве
							// или пока не найдем символ перевода на новую строку, или пока не найдем закрывающую кавычку
						for (i = 0; i < maxLength; i++)
						{

							// Считываем очередной символ
							if ((c = getchar()) == '\n')
							{
								ret = -4; // Сохраняем
								break; // Выходим из
							}
							// Если нашли закрывающую
							else if (c == '\"')
								break; // Выходим из
							str[i] = c; // Каждое
						}
						str[i] = 0; // В конец строки

					}
					// Если была считана не двойная кавычка
					else
					{
						ret = -4; // Сохраняем ошибку
					}
				}
				// Если на предыдущем шаге не было ошибок
				if (ret == 0)
					ret = file_write(buffer, str); // Вызываем функцию записи в файл и передаем в нее имя файла и считанную строку
				// Проверяем значение переменной ret на наличие ошибки
				switch (ret)
				{
				case 0: // Ошибка: в файловой системе не существует файла с заданным именем
					printf("\nВведено имя несуществующего 							файла.\n");
					break;
				case -1: // Ошибка: введено имя большей длины, чем максимальная разрешенная длина хранится ни одного файла
					printf("\nЗадано слишком длинное имя.\n");
					break;
				case -2: // Ошибка: в файловой системе не разрешенный размер

					printf("\nВ файловой системе нет файлов.\n");
					break;
				case -3: // Ошибка: файл уже имеет максимальный
					printf("\nФайл имеет максимальный размер, добавление невозможно.\n");
					break;
				case -4: // Ошибка: строка для добавления введена неправильно или не введена вообще

					printf("\nНеправильный формат строки для добавления.\n");
					break;

				default: // Без ошибки: файл изменен успешно
					printf("\nСодержимое файла успешно обновлено. ID = % d.\n", ret);
					break;
				}
			}
		}
		// Если введена команда cleanup
		else if (strcmp(buffer, "cleanup") == 0)
		{

			char form;
			printf("\nВы точно хотите отформатироватьфайловую систему ? [Y / N] : ");
			scanf("%c", &form);
			scanf("%c", &form);
			switch (form)
			{
			case 'Y': // Согласие
				fs_clear(); // Вызываем функцию форматирования файловой системы

				printf("\nФайловая система отформатирована успешно\n");
				break;
			case 'N': //
				printf("\nФорматирование отменено.\n");
				break;
			}
		}
		// Если введена команда exit
		else if (strcmp(buffer, "exit") == 0)
		{
			printf("\nЗавершение работы\n");
			flag = 0; // Сбрасываем флаг в 0, что приведет к завершению основного цикла программы
		}
		// Если введено что угодно другое
		else
		{
			printf("\nКоманда не найдена.\nПовторите ввод.\n");
		}
	}
	close(fd); // Закрываем файловую систему
	return 0; // Завершаем программу
}

//вспомогательные функции
// Считать суперблок
int readSuperBlock(SuperBlock* sb)
{
	lseek(fd, shift_superBlock, SEEK_SET); // Смещаемся до супер блока
	read(fd, sb, sizeof(SuperBlock)); // Читаем супер блок из памяти в
	return 0;
}

// Считать inode с заданным id
int readINode(INode* inode, int id)
{
	SuperBlock sb;
	// Проверяем правильность id
	if (id > inodeNum || id < 1)
		return -1; // Возвращаем ошибку, если передан неправильный id
	readSuperBlock(&sb); // Считываем супер блок
	lseek(fd, sb.shift_tableINode + sizeof(INode) * (id - 1), SEEK_SET); // Смещаемся до нужного inode - а
	read(fd, inode, sizeof(INode)); // Читаем inode из памяти в структуру inode
	return 0;
}
// Считать имя принадлежащее inode-у с заданным id
int readName(char* name, int id)
{
	SuperBlock sb;
	// Проверяем правильность id
	if (id > inodeNum || id < 1)
		return -1; // Возвращаем ошибку, если передан неправильный id
	readSuperBlock(&sb); // Считываем супер блок
	lseek(fd, sb.shift_nameTable + nameLen * (id - 1), SEEK_SET); // Смещаемся до нужного имени
	read(fd, name, nameLen); // Читаем имя из памяти в массив name
	return 0;
}
// Считать блок размером 2048 байт, находящийся по заданному смещению
int readBlock(char* buffer, uint32_t shift)
{
	// Проверяем правильность смещения
	if (shift > 11264 + blockSize * (blockNum - 1) || shift < 1024)
		return -1; // Возвращаем ошибку, если передано неправильное
	lseek(fd, shift, SEEK_SET); // Смещаемся по заданному смещению
	read(fd, buffer, blockSize); // Читаем блок данных из памяти в массив buffer
	return 0;
}
// Записать суперблок
int writeSuperBlock(SuperBlock* sb)
{
	lseek(fd, shift_superBlock, SEEK_SET); // Смещаемся до супер блока
	write(fd, sb, sizeof(SuperBlock)); // Записываем супер блок из структуры sb в память
	return 0;
}
// Записать inode с заданным id
int writeINode(INode* inode, int id)
{
	SuperBlock sb;
	// Проверяем правильность id
	if (id > inodeNum || id < 1)
		return -1; // Возвращаем ошибку, если передан неправильный id
	readSuperBlock(&sb); // Считываем супер блок
	lseek(fd, sb.shift_tableINode + sizeof(INode) * (id - 1), SEEK_SET); // Смещаемся до нужного inode - а
	write(fd, inode, sizeof(INode)); // Записываем inode из структуры inode в память
	return 0;
}
// Записать имя принадлежащее inode-у с заданным id
int writeName(char* name, int id)
{
	SuperBlock sb;
	// Проверяем правильность id
	if (id > inodeNum || id < 1)
		return -1; // Возвращаем ошибку, если передан неправильный id
	readSuperBlock(&sb); // Считываем супер блок
	lseek(fd, sb.shift_nameTable + nameLen * (id - 1), SEEK_SET); // Смещаемся до нужного имени
	write(fd, name, nameLen); // Записываем имя из массива name в память
	return 0;
}
// Записать блок размером 2048 байт в позицию, заданную смещением
int writeBlock(char* buffer, uint32_t shift)
{
	// Проверяем правильность смещения
	if (shift > 11264 + blockSize * (blockNum - 1) || shift < 1024)
		return -1; // Возвращаем ошибку, если передано неправильное
	lseek(fd, shift, SEEK_SET); // Смещаемся по заданному смещению
	write(fd, buffer, blockSize); // Записываем блок данных из массива buffer в
	return 0;

}

// Освободить inode с заданным id в таблице
int clearINode(int id)
{
	INode inode;
	// Проверяем правильность id
	if (id > inodeNum || id < 1)
		return -1; // Возвращаем ошибку, если передан неправильный id
	// Обнуляем все поля в inode
	memset(&inode, 0, sizeof(INode));
	inode.id = id; // Задаем id inode-а
	writeINode(&inode, id); // Записываем получившийся inode в
	return 0;
}

// Удалить запись с именем файла, принадлежащую inode-у с заданным id
int clearName(int id)
{
	char name[nameLen];
	// Проверяем правильность id
	if (id > inodeNum || id < 1)
		return -1; // Возвращаем ошибку, если передан неправильный id
	// Обнуляем массив name
	memset(name, 0, nameLen);
	writeName(name, id);// Записываем пустое имя в память
	return 0;
}
// Найти имя в таблице имен
int findName(char* name)
{
	char curName[nameLen];
	// Если длина заданного имени больше разрешенной длины
	if (strlen(name) > nameLen)
		return -1; // Возвращаем ошибку
	// Проходим по всем именам
	for (int i = 1; i <= inodeNum; i++)
	{
		readName(curName, i); // Читаем текущее имя из памяти
		// Если переданное имя совпадает с именем взятым из памяти
		if (memcmp(name, curName, nameLen) == 0)
			return i; // Возвращаем id inode-а
	}
	return 0; // Возвращаем 0, если не нашли совпадений
}
// Найти первый встретившийся не занятый inode
int findFreeINode()
{
	SuperBlock sb;
	char bitmap[blockSize];
	int pos = 0;
	readSuperBlock(&sb); // Считываем суперблок
	readBlock(bitmap, sb.shift_inodeBitMap); // Считываем битовую карту занятости inode - ов
	// В цикле проходим элементы битовой карты
	for (int i = 0; i < inodeNum; i++)
	{
		// Если текущий inode свободен, то есть элемент битовой карты равен 0
		if (bitmap[i] == 0)
		{
			pos = i + 1; // Запоминаем номер нулевого бита
			break; // Завершаем цикл
		}
	}
	return pos; // Возвращаем номер найденного нулевого бита, либо 0 если ничего не нашли
}
// Найти первый встретившийся не занятый блок данных
int findFreeDataBlock()
{
	SuperBlock sb;
	char bitmap[blockSize];
	int pos = 0;
	readSuperBlock(&sb); // Считываем суперблок
	readBlock(bitmap, sb.shift_dataBlockBitMap); // Считываем битовую карту занятости блоков данных
	// В цикле проходим элементы битовой карты
	for (int i = 0; i < blockNum; i++)
	{
		// Если текущий блок данных свободен, то есть элемент битовой карты равен 0
		if (bitmap[i] == 0)
		{
			pos = i + 1; // Запоминаем номер нулевого бита
			break; // Завершаем цикл
		}
	}
	return pos; // Возвращаем номер найденного нулевого бита, либо 0 если ничего не нашли
}
// Инвертировать бит в inodeBitMap с номером id
int invertBitINode(int id)
{
	SuperBlock sb;
	char bitmap[blockSize];
	// Проверяем правильность id
	if (id > inodeNum || id < 1)
		return -1; // Возвращаем ошибку, если передан неправильный id
	readSuperBlock(&sb); // Считываем суперблок
	readBlock(bitmap, sb.shift_inodeBitMap); // Считываем битовую карту занятости inode - ов
	bitmap[id - 1] = !bitmap[id - 1]; // Инвертируем нужный элемент битовой карты
	writeBlock(bitmap, sb.shift_inodeBitMap); // Записываем битовую карту занятости inode - ов в память
	return 0;
}
// Инвертировать бит в dataBlockBitMap с номером num
int invertBitDataBlock(int num)
{
	SuperBlock sb;
	char bitmap[blockSize];
	// Проверяем правильность num
	if (num > blockNum || num < 1)
		return -1; // Возвращаем ошибку, если передан неправильный num
	readSuperBlock(&sb); // Считываем суперблок
	readBlock(bitmap, sb.shift_dataBlockBitMap); // Считываем битовую карту занятости блоков данных
	bitmap[num - 1] = !bitmap[num - 1]; // Инвертируем нужный элемент битовой карты
	writeBlock(bitmap, sb.shift_dataBlockBitMap); // Записываем битовую карту занятости inode - ов в память
	return 0;
}
// Выделить память под ФС
int makeFileSystem()
{
	char buf[1024]; // Создаем буффер на 1024 байта
	memset(buf, '\0', 1024); // Обнуляем буффер
	lseek(fd, shift_superBlock, SEEK_SET); // Смещаемся в начало дискового пространства, выделенного под файловую систему
	// Проходим по всем блокам в файловой системе
	for (int i = 0; i < blockNum + 5; i++)
	{
		// Обнуляем в памяти текущий блок
		write(fd, buf, 1024);
		write(fd, buf, 1024);
	}
}
// Открыть ФС, создать пустую ФС если не удалось открыть
int openFileSystem()
{
	fd = open(fsfolder, O_RDWR); // Открываем файл для доступа к памяти
	// Если не удалось открыть
	if (fd == -1)
	{
		fd = open(fsfolder, O_RDWR | O_CREAT, S_IWRITE | S_IREAD); // Создаем файл и открываем
		// Если не удалось создать и открыть
		if (fd == -1)
		{
			printf("Ошибка выделения памяти под файловую систему\n"); //
			return -1; // Возвращаем ошибку
		}
		// Если файл создался удачно
		fs_clear(); // Форматируем новую файловую систему
	}
	return 0;
}
// Вывести список файлов, хранящихся в файловой системе
void printFS(int arg)
{
	char curName[nameLen];
	INode inode;
	uint32_t num = 0;
	// Выводим список всех файлов хранящихся в файловой системе
	for (int i = 1; i <= inodeNum; i++)
	{
		readName(curName, i); // Считываем очередное имя из таблицы имен
		// Если считана не пустая строка
		if (curName[0] != 0)
		{
			printf(" %s", curName); // Выводим имя на
			// Если аргумент задан равным 1, то наджо вывести подробную
			if (arg == 1)
			{
				readINode(&inode, i); // Считываем inode
				printINode(&inode); // Выводим
			}
			num++; // Увеличиваем на 1 счетчик найденных файлов
		}
	}
	printf("\nВсего найдено файлов: %d\n", num); // Выводим общее количество найденных файлов
}
// Вывести справочное сообщение
void printHelpMessage()
{
	// Тут в каждой строчке просто выводим часть справочного сообщения
	printf("\nВывод подробной информации о командах в Файловой Системе\n");
	printf("\x1b[1;36m[create] [ls] [rem] [out] [echo] [cleanup] [exit]\x1b[0m\n\n");
	printf(" ├ \x1b[1;36mcreate \x1b[31mFILE\x1b[0m Создать файл с введённым именем.\n");
	printf(" │\n");
	printf(" ├ \x1b[1;36mls\x1b[0m Вывод списка всех файлов.\n");
	printf(" ├ \x1b[1;36mls \x1b[31m-det\x1b[0m Подробный вывод списка всех файлов.\n");
	printf(" │\n");
	printf(" ├ \x1b[1;36mrem \x1b[31mFILE\x1b[0m Удалить файл с введённым именем.\n");
	printf(" │\n");
	printf(" ├ \x1b[1;36mecho \x1b[31mFILE \"Text\"\x1b[0m Дописывает информацию в конец файла.\n");
	printf(" │\n");
	printf(" ├ \x1b[1;36mout \x1b[31mFILE\x1b[0m Вывод содержимого файла с введённым именем.\n");
	printf(" │\n");
	printf(" ├ \x1b[1;36mcleanup\x1b[0m Форматирует файловую систему.\n");
	printf(" │\n");
	printf(" └ \x1b[1;36mexit\x1b[0m Сохранение текущего состояния файловой системы\n");
	printf(" и выход из подпрограммы работы с ней.\n");
}
// Вывести содержимое inode-а на экран
void printINode(INode* inode)
{
	printf("\t%4d ", inode->id); // Выводим id inode-а
	printf("%d ", inode->uid); // Выводим id пользователя создавшего
	printf("%5d ", inode->size); // Выводим размер файла в байтах
	printf("%s", ctime(&inode->createTime)); // Выводим дату и время создания
}

/*============================== ОСНОВНЫЕ ФУНКЦИИ
==============================*/
// Форматирование ФС
int fs_clear()
{
	SuperBlock sb;
	INode inode;
	makeFileSystem(); // Обнуляем дисковое пространство, выделенное под файловую систему
	// Инициализируем все поля супер блока необходимыми значениями
	sb.shift_tableINode = shift_superBlock + blockSize;
	sb.shift_inodeBitMap = sb.shift_tableINode + blockSize;
	sb.shift_dataBlockBitMap = sb.shift_inodeBitMap + blockSize;
	sb.shift_nameTable = sb.shift_dataBlockBitMap + blockSize;
	sb.shift_dataGroup = sb.shift_nameTable + blockSize;
	sb.shift_freeDataBlock = sb.shift_dataGroup;
	sb.num_inode = inodeNum;
	sb.num_dataBlock = blockNum;
	sb.cnt_freeInode = sb.num_inode;
	sb.cnt_freeDataBlock = sb.num_dataBlock;
	writeSuperBlock(&sb); // Записываем супер блок в память
	memset(&inode, 0, sizeof(INode)); // Обнуляем все поля inode-а
	for (int i = 1; i <= inodeNum; i++)
	{
		inode.id = i; // Задаем id очередного inode-a
		writeINode(&inode, i); // Записываем текущий inode в память
	}
}
// Создание нового файла
int file_create(char* name)
{
	SuperBlock sb;
	INode inode;
	uint32_t inodeID = 0;
	uint32_t datablockNum = 0;
	char normalizeName[nameLen];
	// Если длина заданного имени больше разрешенной длины
	if (strlen(name) > nameLen)
		return -1; // Возвращаем ошибку
	// Приводим имя к удобному для работы виду
	memset(normalizeName, 0, nameLen);
	memmove(normalizeName, name, strlen(name));
	inodeID = findName(normalizeName); // Ищем заданное имя в таблице имен файлов
	// Если уже существует файл с таким именем
	if (inodeID != 0)
		return -2; // Возвращаем ошибку
	readSuperBlock(&sb); // Считываем супер блок из памяти
	// Если все inode-ы заняты
	if (sb.cnt_freeInode == 0)
		return 0; // Возвращаем ошибку
	inodeID = findFreeINode(); // Находим номер первого свободного inode-а
	// Если не удалось считать inode с найденным id
	if (readINode(&inode, inodeID) == -1)
		return 0; // Возвращаем ошибку
	invertBitINode(inodeID); // Меняем бит в битовой карте занятости inode-ов номер в inode
	// Вычисляем позицию первого свободного блока данных и записываем его
	inode.dataBlockPos[0] = (sb.shift_freeDataBlock - 11264) / blockSize + 1;
	invertBitDataBlock(inode.dataBlockPos[0]); // Меняем бит в битовой карте занятости блоков данных
	writeName(normalizeName, inodeID); // Записываем имя файла в таблицу имен
	// Заполняем оставшиеся поля inode-а
	inode.uid = getuid();
	inode.size = 0;
	time(&inode.createTime);
	writeINode(&inode, inodeID); // Записываем полученный inode в память
	// Изменяем статистическую информацию в супер блоке
	sb.cnt_freeDataBlock -= 1; // Уменьшаем количество свободных блоков данных
	sb.cnt_freeInode -= 1; // Уменьшаем количество свободных inode - ов
	datablockNum = findFreeDataBlock(); // Находим новый первый свободный блок данных
	// Если возвращен номер блока равный 0, значит свободных блоков больше нет
	if (datablockNum == 0)
		sb.shift_freeDataBlock = 11264 + blockSize * blockNum; // Сдвиг до первого свободного блока данных приравниваем к максимальному доступному в файловой системе адресу
	// Если возвращен не 0, значит свободный блок найден успешно
	else
		sb.shift_freeDataBlock = 11264 + blockSize * (datablockNum - 1); // Высчитываем сдвиш до этого блока и сохраняем в супер блоке
	writeSuperBlock(&sb); // Записываем обновленный супер блок в память
	return inodeID; // Возвращаем id inode-а созданного файла
}
// Удаление существующего файла
int file_delete(char* name)
{
	SuperBlock sb;
	INode inode;
	uint32_t inodeID = 0;
	char normalizeName[nameLen];
	// Если длина заданного имени больше разрешенной длины
	if (strlen(name) > nameLen)
		return -1; // Возвращаем ошибку
	readSuperBlock(&sb); // Считываем супер блок
	// Если все inode-ы свободны
	if (sb.cnt_freeInode == inodeNum)
		return -2; // Возвращаем ошибку
	// Приводим имя к удобному для работы виду
	memset(normalizeName, 0, nameLen);
	memmove(normalizeName, name, strlen(name));
	inodeID = findName(normalizeName); // Ищем заданное имя в таблице имен файлов
	// Если имя не найдено
	if (inodeID == 0)
		return 0; // Возвращаем ошибку
	clearName(inodeID); // Если имя найдено, то удаляем запись с ним из таблицы имен
	readINode(&inode, inodeID); // Считываем inode
	// В цикле проходим по всем доступным данному файлу блокам данных
	for (int i = 0; i < 3; i++)
	{
		char buffer[blockSize]; // Создаем буффер размером как блок данных = 2048 байт
		uint32_t shift;
		// Если очередной блок данных не был выделен для данного файла
		if (inode.dataBlockPos[i] == 0)
			break; // Завершаем цикл
		memset(buffer, 0, blockSize); // Обнуляем буффер
		shift = 11264 + blockSize * (inode.dataBlockPos[i] - 1); // Рассчитываем смещение до текущего очищаемого блока данных
		writeBlock(buffer, shift); // Записываем обнуленный буффер в память на место данных удаляемого файла
		invertBitDataBlock(inode.dataBlockPos[i]); // Обнуляем бит с номером текущего блока данных в битовой карте занятости блоков данных
		sb.cnt_freeDataBlock++; // Увеличиваем на 1 количество свободных блоков данных в файлвой системе
	}
	clearINode(inodeID); // Освобождаем inode
	invertBitINode(inodeID); // Обнуляем бит с номером текущего inode-а в битовой карте занятости inode - ов
	// Изменяем статистическую информацию в супер блоке
	sb.cnt_freeInode += 1; // Увеличиваем количество свободных inode - ов в файловой системе
	sb.shift_freeDataBlock = 11264 + blockSize * (findFreeDataBlock() - 1); // Высчитываем смещение до нового первого свободного блока дынных
	writeSuperBlock(&sb); // Записываем обновленный супер блок в память
	return inodeID; // Возвращаем id inode-а удаленного файла
}
// Вывод содержимого существующего файла на экран
int file_read(char* name)
{
	SuperBlock sb;
	INode inode;
	uint32_t inodeID = 0;
	char normalizeName[nameLen];
	// Если длина заданного имени больше разрешенной длины
	if (strlen(name) > nameLen)
		return -1; // Возвращаем ошибку
	readSuperBlock(&sb); // Считываем супер блок
	// Если все inode-ы свободны
	if (sb.cnt_freeInode == inodeNum)
		return -2; // Возвращаем ошибку
	// Приводим имя к удобному для работы виду
	memset(normalizeName, 0, nameLen);
	memmove(normalizeName, name, strlen(name));
	inodeID = findName(normalizeName); // Ищем заданное имя в таблице имен файлов
	// Если имя не найдено
	if (inodeID == 0)
		return 0; // Возвращаем ошибку
	readINode(&inode, inodeID); // Считываем inode
	// В цикле проходим по всем доступным данному файлу блокам данных
	for (int i = 0; i < 3; i++)
	{
		char buffer[blockSize + 1]; // Создаем буффер размером какблок данных + 1байт = 2049 байт
		uint32_t shift;
		// Если очередной блок данных не был выделен для данного файла
		if (inode.dataBlockPos[i] == 0)
			break; // Завершаем цикл
		shift = 11264 + blockSize * (inode.dataBlockPos[i] - 1); // Рассчитываем смещение до текущего блока данных
		memset(buffer, 0, blockSize + 1); // Очищаем буффер
		readBlock(buffer, shift); // Читаем блок данных в буффер данных в терминал
		printf("%s", buffer); // Выводим считанный блок
	}
	return inodeID;
}// Возвращаем id inode-а файла который был выведен на
// Добавление строки в конец существующего файла
int file_write(char* name, char* str)
{

	SuperBlock sb;
	INode inode;
	uint32_t inodeID = 0;
	uint32_t strLen = 0, shift = 0, strShift = 0, blockShift = 0, curBlock = 0;
	char normalizeName[nameLen];
	char dataBlock[blockSize]; // Создаем буффер размером как блок данных =
	// Если длина заданного имени больше разрешенной длины
	if (strlen(name) > nameLen)
		return -1; // Возвращаем ошибку
	readSuperBlock(&sb); // Считываем супер блок из памяти
	// Если все inode-ы свободны
	if (sb.cnt_freeInode == inodeNum)
		return -2; // Возвращаем ошибку
	// Приводим имя к удобному для работы виду
	memset(normalizeName, 0, nameLen);
	memmove(normalizeName, name, strlen(name));
	inodeID = findName(normalizeName); // Ищем заданное имя в таблице имен файлов
	// Если имя не найдено
	if (inodeID == 0)
		return 0; // Возвращаем ошибку
	readINode(&inode, inodeID); // Считываем inode
	// Если файл уже максимального размера
	if (inode.size == maxLength)
		return -3; // Возвращаем ошибку
	curBlock = inode.size / blockSize; // Вычисляем какой по номеру блок был выделен последним для данного файла
	shift = 11264 + blockSize * (inode.dataBlockPos[curBlock] - 1); // Вычисляем смещения до этого блока данных
	readBlock(dataBlock, shift); // Считываем этот блок данных в буффер
	blockShift = inode.size % blockSize; // Вычисляем смещение в блоке данных до последнего байта в который записаны данные
	strLen = strlen(str); // Вычисляем длину дописываемой к файлу строки
	// В цикле пока не дойдем до конца добавляемой строки или пока не достигнем максимального размера файла
	for (strShift = 0; strShift < strLen && inode.size < maxLength; strShift++,
		blockShift++)
	{
		// Если место в текущем блоке закончилось, а максимальный размер файла не был достигнут
		if (inode.size % blockSize == 0 && inode.size / blockSize > curBlock && curBlock != 2)
		{
			writeBlock(dataBlock, shift);// Записываем заполненный блок в
			blockShift = 0; // Обнуляем смещение внуцтри блока, потому что продолжим записывать в новом блоке с нулевого байта
			curBlock++; // Увеличиваем на 1 номер текущего выделенного блока для данного файла
			inode.dataBlockPos[curBlock] = findFreeDataBlock(); // Находим позицию нового свободного блока данных
			invertBitDataBlock(inode.dataBlockPos[curBlock]); // Ставим в 1 бит с номером найденного блока данных в битовой карте занятости блоков данных
			shift = 11264 + blockSize * (inode.dataBlockPos[curBlock] - 1);
			// Находим смещение до выделенного блока данных
			sb.cnt_freeDataBlock--; // Уменьшаем на 1 количество свободных блоков данных в файловой системе
		}
		dataBlock[blockShift] = str[strShift]; // Записываем текущий символ из добавляемой строки в текущий блок данных
		inode.size++; // Увеличивем на 1 размер файла
	}
	writeBlock(dataBlock, shift);// Записываем измененный блок данных в память
	writeINode(&inode, inodeID); // Записываем inode измененного файла в память
	// Изменяем статистическую информацию в супер блоке
	sb.shift_freeDataBlock = 11264 + blockSize * (findFreeDataBlock() - 1); // Вычисляем смещение до нового первого свободного блока данных
	writeSuperBlock(&sb); // Записываем обновленный супер блок в память
	return inodeID; // Возвращаем id inode-а файла, который был изменен
}