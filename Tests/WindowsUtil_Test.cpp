#include "catch_amalgamated.hpp"

#include "Runtime/Utility/WindowsUtil.h"

TEST_CASE(
    "WindowsUtil",
    "[unit][utility]")
{

    SECTION("ToString")
    {
        SECTION("check empty string")
        {
            CHECK(WindowsUtil::ToString(L"") == "");
        }

        SECTION("check CJK character")
        {
            const FWString Original = L"테스트 테스트";
            const FString UTF8 = WindowsUtil::ToString(Original);

            REQUIRE_FALSE(UTF8.empty());
            CHECK(UTF8 == "테스트 테스트");
        }

        SECTION("check emoji")
        {
            const FWString Original = L"🕹🌟👍";
            const FString UTF8 = WindowsUtil::ToString(Original);

            REQUIRE_FALSE(UTF8.empty());
            CHECK(UTF8 == "🕹🌟👍");
        }
    }

    SECTION("ToWString")
    {
        SECTION("check empty string")
        {
            CHECK(WindowsUtil::ToWString("") == L"");
        }

        SECTION("check CJK character")
        {
            const FString Original = "테스트 테스트";
            const FWString Wide = WindowsUtil::ToWString(Original);

            REQUIRE_FALSE(Wide.empty());
            CHECK(Wide == L"테스트 테스트");
        }

        SECTION("check emoji")
        {
            const FString Original = "🕹🌟👍";
            const FWString Wide = WindowsUtil::ToWString(Original);

            REQUIRE_FALSE(Wide.empty());
            CHECK(Wide == L"🕹🌟👍");
        }
    }
}