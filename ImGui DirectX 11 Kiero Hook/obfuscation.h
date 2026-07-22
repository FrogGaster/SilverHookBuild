#pragma once

#include <cstddef>
#include <cstdint>

namespace obfuscation
{
    constexpr std::uint8_t keyByte(std::uint32_t seed, std::size_t index) noexcept
    {
        std::uint32_t value = seed + static_cast<std::uint32_t>(index * 0x45D9F3Bu);
        value ^= value >> 16;
        value *= 0x45D9F3Bu;
        value ^= value >> 16;
        return static_cast<std::uint8_t>((value & 0xFFu) | 1u);
    }

    template <std::size_t Size, std::uint32_t Seed>
    class XorString
    {
    public:
        constexpr explicit XorString(const char (&text)[Size]) noexcept : encrypted_{}, decrypted_{}
        {
            for (std::size_t index = 0; index < Size; ++index)
                encrypted_[index] = static_cast<std::uint8_t>(text[index]) ^ keyByte(Seed, index);
        }

        __declspec(noinline) const char* get() const noexcept
        {
            volatile const std::uint8_t* source = encrypted_;
            for (std::size_t index = 0; index < Size; ++index)
                decrypted_[index] = static_cast<char>(source[index] ^ keyByte(Seed, index));
            return decrypted_;
        }

    private:
        std::uint8_t encrypted_[Size];
        mutable char decrypted_[Size];
    };
}

#define OBFUSCATE_IMPL(text, seed) ([]() -> const char* { \
    static obfuscation::XorString<sizeof(text), seed> value(text); \
    return value.get(); \
}())

#define OBFUSCATE_SEED(line, counter) \
    (static_cast<std::uint32_t>((line) * 0x9E37u + (counter) * 0x79B9u + 0xA5u))

#define OBFUSCATE(text) OBFUSCATE_IMPL(text, OBFUSCATE_SEED(__LINE__, __COUNTER__))
#define FIND_PATTERN(pattern, mask) FindPattern(OBFUSCATE(pattern), OBFUSCATE(mask))
