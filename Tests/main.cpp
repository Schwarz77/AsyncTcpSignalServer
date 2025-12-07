#include "gtest/gtest.h"

// Основная функция для запуска всех тестов.
// Она инициализирует Google Test и запускает все определенные тесты.
int main(int argc, char **argv) 
{
    // Инициализация Google Test framework
    ::testing::InitGoogleTest(&argc, argv);
    
    // Запуск всех тестов
    return RUN_ALL_TESTS();
}