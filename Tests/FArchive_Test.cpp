#include "catch_amalgamated.hpp"

#include "Runtime/Engine/FArchive.h"
#include "Runtime/Material/FRasterizerDesc.h"

TEST_CASE(
    "FArchive",
    "[unit][archive]")
{

    SECTION("set and get test")
    {

        SECTION("numbers")
        {
            FArchive Prev;

            Prev.SetInt32("Int32", 2147483647);
            Prev.SetUInt32("UInt32", 4294967295);
            Prev.SetFloat("Float", 1.0);
            Prev.SetDouble("Double", 3.14159265359);

            auto Json = Prev.GetJSON();
            FArchive Next{ Json };

            CHECK(Next.GetInt32("Int32") == 2147483647);
            CHECK(Next.GetUInt32("UInt32") == 4294967295);
            CHECK(Next.GetFloat("Float") == 1.0);
            CHECK(Next.GetDouble("Double") == 3.14159265359);
        }

        SECTION("string")
        {
            FArchive Prev;

            Prev.SetString("ASCII", "hello, world");
            Prev.SetString("CJK", "크래프톤 정글 테크랩 4기 화이팅!");
            Prev.SetString("Emoji", "🎮✨");

            auto Json = Prev.GetJSON();
            FArchive Next{ Json };

            CHECK(Next.GetString("ASCII") == "hello, world");
            CHECK(Next.GetString("CJK") == "크래프톤 정글 테크랩 4기 화이팅!");
            CHECK(Next.GetString("Emoji") == "🎮✨");
        }

        SECTION("array")
        {
            FArchive Prev;

            TArray<int32> IntArray{ -1, 0, 1, 2147483647 };
            TArray<FString> StringArray{ "hello", "크래프톤 정글", "🎮" };

            Prev.SetArray("IntArray", IntArray);
            Prev.SetArray("StringArray", StringArray);

            auto Json = Prev.GetJSON();
            FArchive Next{ Json };

            CHECK(Next.GetArray<int32>("IntArray") == IntArray);
            CHECK(Next.GetArray<FString>("StringArray") == StringArray);
        }

        SECTION("vector")
        {
            FArchive Prev;

            FVector4 Vec4{ 1.0f,  2.0f, 3.0f, 4.0f };
            FVector  Vec3{ 1.0f, -1.0f, 3.14f };
            FVector2 Vec2{ 5.0f, -1.5f };

            Prev.SetVector4("Vec4", Vec4);
            Prev.SetVector("Vec3", Vec3);
            Prev.SetVector2("Vec2", Vec2);

            auto Json = Prev.GetJSON();
            FArchive Next{ Json };

            CHECK(Next.GetVector4("Vec4") == Vec4);
            CHECK(Next.GetVector("Vec3") == Vec3);
            CHECK(Next.GetVector2("Vec2") == Vec2);
        }

        SECTION("enum")
        {
            FArchive Prev;

            TMap<ERasterizerFillMode, FString> EnumToString
            {
                { ERasterizerFillMode::Solid, "Solid" },
                { ERasterizerFillMode::Wireframe, "Wireframe" },
            };

            TMap<FString, ERasterizerFillMode> StringToEnum
            {
                { "Solid", ERasterizerFillMode::Solid },
                { "Wireframe", ERasterizerFillMode::Wireframe },
            };

            // 오류 동작
            SECTION("incorrect enum value")
            {
                CHECK_THROWS(Prev.SetEnum("FillMode", static_cast<ERasterizerFillMode>(999), EnumToString));

                Prev.SetString("FillMode", "Unknown");

                auto Json = Prev.GetJSON();
                FArchive Next{ Json };

                CHECK_THROWS(Next.GetEnum("FillMode", StringToEnum) == ERasterizerFillMode::Wireframe);
            }

            // 정상 동작
            SECTION("correct enum value")
            {
                Prev.SetEnum("FillMode", ERasterizerFillMode::Wireframe, EnumToString);

                auto Json = Prev.GetJSON();
                FArchive Next{ Json };

                CHECK(Next.GetEnum("FillMode", StringToEnum) == ERasterizerFillMode::Wireframe);
            }
        }

        SECTION("nested archive")
        {
            FArchive Prev;

            FVector  Vec3{ 1.0f, -1.0f, 3.14f };

            FArchive PrevNest;
            PrevNest.SetInt32("Int32", 2147483647);
            PrevNest.SetString("CJK", "크래프톤 정글 테크랩 4기 화이팅!");
            PrevNest.SetVector("Vec3", Vec3);

            Prev.SetArchive("Nest", PrevNest);

            auto Json = Prev.GetJSON();
            FArchive Next{ Json };

            FArchive NextNest = Next.GetArchive("Nest");

            CHECK(NextNest.GetInt32("Int32") == 2147483647);
            CHECK(NextNest.GetString("CJK") == "크래프톤 정글 테크랩 4기 화이팅!");
            CHECK(NextNest.GetVector("Vec3") == Vec3);
        }

        SECTION("nested archive array")
        {
            FArchive First;
            First.SetInt32("Index", 1);
            First.SetString("Name", "First");

            FArchive Second;
            Second.SetInt32("Index", 2);
            Second.SetString("Name", "Second");

            FArchive Prev;
            Prev.SetArchiveArray("Items", TArray<FArchive>{ First, Second });

            auto Json = Prev.GetJSON();
            FArchive Next{ Json };
            TArray<FArchive> Items = Next.GetArchiveArray("Items");

            REQUIRE(Items.size() == 2);
            CHECK(Items[0].GetInt32("Index") == 1);
            CHECK(Items[0].GetString("Name") == "First");
            CHECK(Items[1].GetInt32("Index") == 2);
            CHECK(Items[1].GetString("Name") == "Second");
        }

        SECTION("is null")
        {
            FArchive Archive;
            Archive.SetNull("ExplicitNull");
            Archive.SetString("Present", "value");

            CHECK(Archive.IsNull("Missing"));
            CHECK(Archive.IsNull("ExplicitNull"));
            CHECK_FALSE(Archive.IsNull("Present"));
        }

    }
}
