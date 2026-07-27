#include "argList.H"
#include "fileName.H"      // rmDir, mkDir, isDir и т.д.
#include "Pstream.H"

#include <unistd.h>
#include <limits.h>

using namespace Foam;

int main(int argc, char *argv[])
{
    // Инициализируем разбор аргументов командной строки
    argList args(argc, argv);

    // Получаем путь к кейсу через globalCaseName(), чтобы избежать конфликта с макросом #define case
    const fileName caseDir = args.globalCaseName();

    Info << "Target directory to create: " << caseDir << nl << endl;

    // Если папка уже существует — можно либо выйти, либо ничего не делать.
    // Здесь выбираем «ничего не делать» и просто сообщаем.
    if (isDir(caseDir))
    {
        Info << "Directory '" << caseDir << "' already exists, nothing to do." << nl << endl;
        return 0;
    }

    // Создаём директорию. mkDir в OpenFOAM рекурсивен (аналог mkdir -p)
    bool success = mkDir(caseDir);

    if (success)
    {
        Info << "Successfully created directory: " << caseDir << nl << endl;
    }
    else
    {
        FatalErrorInFunction
            << "Failed to create directory '" << caseDir << "'. "
            << "Check permissions and parent directories." << nl
            << exit(FatalError);
    }

    return 0;
}
