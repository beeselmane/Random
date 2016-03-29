/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* CXUnicodeConverters.c - Unicode Converters for CXString         */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* beeselmane - 23.3.2016  - 11:30 PM PST                          */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* beeselmane - 27.3.2016  - 12:00 PM EST                          */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/

#include <CX/CXInternal.h>
#include <CX/CXStringPrivate.h>

static const CXUnicodePoint kCXSurrogateHighBegin  = 0xD800;
static const CXUnicodePoint kCXSurrogateHighFinish = 0xDBFF;
static const CXUnicodePoint kCXSurrogateLowBegin   = 0xDC00;
static const CXUnicodePoint kCXSurrogateLowFinish  = 0xDFFF;

static const UInt8 kCXUTF8ExtraBytes[0x100] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
};

static const UInt32 kCXUTF8ExcessBytes[4] = {
    0,
    (0xC0 <<  6) | (0x80 <<  0),
    (0xE0 << 12) | (0x80 <<  6) | (0x80 <<  0),
    (0xF0 << 18) | (0x80 << 12) | (0x80 <<  6) | (0x80 <<  0),
};

static const UInt8 kCXUTF8FirstByteMasks[5] = {
    0b00000000,
    0b00000000,
    0b11000000,
    0b11100000,
    0b11110000,
};

CXInline UInt8 CXIUTF8FromCodePoint(CXUnicodePoint codepoint, CXCharUTF8 *outputBuffer, CXSize sizeLeft)
{
    UInt8 byteCount;

    if (codepoint < 0x80) {
        (*outputBuffer) = codepoint;
        return 1;
    } else if (codepoint < 0x800) {
        byteCount = 2;
    } else if (codepoint < 0x10000) {
        byteCount = 3;
    } else {
        byteCount = 4;
    }

    if (byteCount > sizeLeft) return 0;

    switch (byteCount)
    {
        case 4: outputBuffer[3] = (codepoint | 0x80) & 0xBF; codepoint >>= 6;
        case 3: outputBuffer[2] = (codepoint | 0x80) & 0xBF; codepoint >>= 6;
        case 2: outputBuffer[1] = (codepoint | 0x80) & 0xBF; codepoint >>= 6;
                outputBuffer[0] =  codepoint | kCXUTF8FirstByteMasks[byteCount];
    }

    return byteCount;
}

CXInline UInt8 CXIUTF16FromCodePoint(CXUnicodePoint codepoint, CXCharUTF16 *outputBuffer, CXSize sizeLeft, OSBoolean byteSwap)
{
    if (codepoint < 0x10000) {
        (*outputBuffer) = (byteSwap ? OSSwap16(codepoint) : codepoint);
        return 1;
    } else {
        if (sizeLeft >= 2) {
            codepoint -= 0x10000;

            outputBuffer[0] = (codepoint >> 10)   + kCXSurrogateHighBegin;
            outputBuffer[1] = (codepoint & 0x3FF) + kCXSurrogateLowBegin;

            if (byteSwap)
            {
                outputBuffer[0] = OSSwap16(outputBuffer[0]);
                outputBuffer[1] = OSSwap16(outputBuffer[1]);
            }

            return 2;
        } else {
            return 0;
        }
    }
}

CXInline CXUnicodePoint CXICodePointFromUTF8(CXCharUTF8 *inputBuffer, CXSize sizeLeft, UInt8 *bytesUsed)
{
    CXCharUTF8 firstByte = (*inputBuffer);
    UInt8 bytes = kCXUTF8ExtraBytes[firstByte];
    CXUnicodePoint codepoint = 0;

    if (sizeLeft < bytes)
    {
        if (bytesUsed) (*bytesUsed) = 0;
        return 0;
    }

    if (bytes > 4)
    {
        if (bytesUsed) (*bytesUsed) = bytes;
        return 0;
    }

    switch (bytes)
    {
        case 3: codepoint += (*inputBuffer++); codepoint <<= 6;
        case 2: codepoint += (*inputBuffer++); codepoint <<= 6;
        case 1: codepoint += (*inputBuffer++); codepoint <<= 6;
        case 0: codepoint += (*inputBuffer++);
    }

    codepoint -= kCXUTF8ExcessBytes[bytes];
    if (bytesUsed) (*bytesUsed) = bytes + 1;
    return codepoint;
}

CXInline CXUnicodePoint CXICodePointFromUTF16(CXCharUTF16 *inputBuffer, CXSize sizeLeft, UInt8 *charsUsed, OSBoolean byteSwap)
{
    CXCharUTF16 firstChar = (*inputBuffer++);
    if (byteSwap) firstChar = OSSwap16(firstChar);

    if (firstChar >= kCXSurrogateHighBegin && firstChar <= kCXSurrogateHighFinish) {
        CXCharUTF16 secondChar = (*inputBuffer);
        if (byteSwap) secondChar = OSSwap16(secondChar);

        if (sizeLeft < 2)
        {
            if (charsUsed) (*charsUsed) = 0;
            return 0;
        }

        if (secondChar <= kCXSurrogateLowBegin || secondChar >= kCXSurrogateLowFinish)
        {
            if (charsUsed) (*charsUsed) = 1;
            return 0;
        }

        if (charsUsed) (*charsUsed) = 2;
        return ((CXUnicodePoint)((firstChar << 10) | secondChar) + 0x10000);
    } else if (firstChar >= kCXSurrogateLowBegin && firstChar <= kCXSurrogateLowFinish) {
        if (charsUsed) (*charsUsed) = 1;
        return 0;
    } else {
        if (charsUsed) (*charsUsed) = 1;
        return (CXUnicodePoint)firstChar;
    }
}

CXSize CXUTF8ToUTF8(CXCharUTF8 *outputBuffer, CXSize outputBufferSize, CXCharUTF8 *inputBuffer, CXSize inputBufferSize, CXSize *usedSize)
{
    CXSize bytesToCopy;

    if (inputBufferSize > outputBufferSize) {
        bytesToCopy = outputBufferSize;
    } else {
        bytesToCopy = inputBufferSize;
    }

    memcpy(outputBuffer, inputBuffer, bytesToCopy);
    if (usedSize) (*usedSize) = bytesToCopy;
    return (bytesToCopy * sizeof(CXCharUTF8));
}

CXSize CXUTF16ToUTF8(CXCharUTF8 *outputBuffer, CXSize outputBufferSize, CXCharUTF16 *inputBuffer, CXSize inputBufferSize, CXSize *usedSize, OSBoolean byteSwap)
{
    outputBufferSize /= sizeof(CXCharUTF8);
    inputBufferSize  /= sizeof(CXCharUTF16);

    CXCharUTF8 *outputBufferOriginal = outputBuffer;
    CXCharUTF16 *inputBufferOriginal = inputBuffer;

    CXCharUTF8 *outputBufferEnd = outputBuffer + outputBufferSize;
    CXCharUTF16 *inputBufferEnd = inputBuffer + inputBufferSize;

    while ((outputBuffer < outputBufferEnd) && (inputBuffer < inputBufferEnd))
    {
        UInt8 charsUsed;
        CXUnicodePoint codepoint = CXICodePointFromUTF16(inputBuffer, inputBufferEnd - inputBuffer, &charsUsed, byteSwap);
        inputBuffer += charsUsed;

        if (!codepoint)
        {
            if (charsUsed) continue;
            else break;
        }

        UInt8 byteCount = CXIUTF8FromCodePoint(codepoint, outputBuffer, outputBufferEnd - outputBuffer);
        outputBuffer += byteCount;
        if (!byteCount) break;
    }

    if (usedSize) (*usedSize) = (outputBuffer - outputBufferOriginal);
    return (inputBuffer - inputBufferOriginal);
}

CXSize CXUTF32ToUTF8(CXCharUTF8 *outputBuffer, CXSize outputBufferSize, CXCharUTF32 *inputBuffer, CXSize inputBufferSize, CXSize *usedSize, OSBoolean byteSwap)
{
    outputBufferSize /= sizeof(CXCharUTF8);
    inputBufferSize  /= sizeof(CXCharUTF32);

    CXCharUTF8 *outputBufferOriginal = outputBuffer;
    CXCharUTF32 *inputBufferOriginal = inputBuffer;
    
    CXCharUTF8 *outputBufferEnd = outputBuffer + outputBufferSize;
    CXCharUTF32 *inputBufferEnd = inputBuffer + inputBufferSize;

    while ((outputBuffer < outputBufferEnd) && (inputBuffer < inputBufferEnd))
    {
        CXCharUTF32 character = (*inputBuffer++);
        if (byteSwap) character = OSSwap32(character);

        if (character >= kCXSurrogateHighBegin && character <= kCXSurrogateLowFinish)
            continue;

        UInt8 byteCount = CXIUTF8FromCodePoint(character, outputBuffer, outputBufferEnd - outputBuffer);
        outputBuffer += byteCount;
        if (!byteCount) break;
    }

    if (usedSize) (*usedSize) = (outputBuffer - outputBufferOriginal);
    return (inputBuffer - inputBufferOriginal);
}

CXSize CXUTF8ToUTF16(CXCharUTF16 *outputBuffer, CXSize outputBufferSize, CXCharUTF8 *inputBuffer, CXSize inputBufferSize, CXSize *usedSize, OSBoolean byteSwap)
{
    outputBufferSize /= sizeof(CXCharUTF16);
    inputBufferSize  /= sizeof(CXCharUTF8);

    CXCharUTF16 *outputBufferOriginal = outputBuffer;
    CXCharUTF8 *inputBufferOriginal = inputBuffer;

    CXCharUTF16 *outputBufferEnd = outputBuffer + outputBufferSize;
    CXCharUTF8 *inputBufferEnd = inputBuffer + inputBufferSize;

    while ((outputBuffer < outputBufferEnd) && (inputBuffer < inputBufferEnd))
    {
        UInt8 bytesUsed;
        CXUnicodePoint codepoint = CXICodePointFromUTF8(inputBuffer, (inputBufferEnd - inputBuffer), &bytesUsed);
        inputBuffer += bytesUsed;

        if (!codepoint)
        {
            if (bytesUsed) continue;
            else break;
        }

        if (codepoint >= kCXSurrogateHighBegin && codepoint <= kCXSurrogateLowFinish)
            continue;

        UInt8 encodedChars = CXIUTF16FromCodePoint(codepoint, outputBuffer, outputBufferEnd - outputBuffer, byteSwap);

        if (encodedChars) {
            outputBuffer += encodedChars;
        } else {
            break;
        }
    }

    if (usedSize) (*usedSize) = ((outputBuffer - outputBufferOriginal) * sizeof(CXCharUTF16));
    return (inputBuffer - inputBufferOriginal);
}

CXSize CXUTF16ToUTF16(CXCharUTF16 *outputBuffer, CXSize outputBufferSize, CXCharUTF16 *inputBuffer, CXSize inputBufferSize, CXSize *usedSize, OSBoolean byteSwap)
{
    CXSize bytesToCopy;

    if (inputBufferSize > outputBufferSize) {
        bytesToCopy = outputBufferSize;
    } else {
        bytesToCopy = inputBufferSize;
    }

    if (byteSwap) {
        memcpy(outputBuffer, inputBuffer, bytesToCopy);
    } else {
        memcpy(outputBuffer, inputBuffer, bytesToCopy);
    }

    if (usedSize) (*usedSize) = bytesToCopy;
    return (bytesToCopy * sizeof(CXCharUTF16));
}

CXSize CXUTF32ToUTF16(CXCharUTF16 *outputBuffer, CXSize outputBufferSize, CXCharUTF32 *inputBuffer, CXSize inputBufferSize, CXSize *usedSize, OSBoolean byteSwap)
{
    outputBufferSize /= sizeof(CXCharUTF16);
    inputBufferSize  /= sizeof(CXCharUTF32);

    CXCharUTF16 *outputBufferOriginal = outputBuffer;
    CXCharUTF32 *inputBufferOriginal = inputBuffer;
    
    CXCharUTF16 *outputBufferEnd = outputBuffer + outputBufferSize;
    CXCharUTF32 *inputBufferEnd = inputBuffer + inputBufferSize;
    
    while ((outputBuffer < outputBufferEnd) && (inputBuffer < inputBufferEnd))
    {
        CXCharUTF32 character = (*inputBuffer++);
        if (byteSwap) character = OSSwap32(character);

        if (character >= kCXSurrogateHighBegin && character <= kCXSurrogateLowFinish)
            continue;

        UInt8 encodedChars = CXIUTF16FromCodePoint(character, outputBuffer, outputBufferEnd - outputBuffer, byteSwap);

        if (encodedChars) {
            outputBuffer += encodedChars;
        } else {
            break;
        }
    }
    
    if (usedSize) (*usedSize) = ((outputBuffer - outputBufferOriginal) * sizeof(CXCharUTF16));
    return (inputBuffer - inputBufferOriginal);
}

CXSize CXUTF8ToUTF32(CXCharUTF32 *outputBuffer, CXSize outputBufferSize, CXCharUTF8 *inputBuffer, CXSize inputBufferSize, CXSize *usedSize, OSBoolean byteSwap)
{
    outputBufferSize /= sizeof(CXCharUTF32);
    inputBufferSize  /= sizeof(CXCharUTF8);

    CXCharUTF32 *outputBufferOriginal = outputBuffer;
    CXCharUTF8 *inputBufferOriginal = inputBuffer;

    CXCharUTF32 *outputBufferEnd = outputBuffer + outputBufferSize;
    CXCharUTF8 *inputBufferEnd = inputBuffer + inputBufferSize;

    while ((outputBuffer < outputBufferEnd) && (inputBuffer < inputBufferEnd))
    {
        UInt8 bytesUsed;
        CXUnicodePoint codepoint = CXICodePointFromUTF8(inputBuffer, inputBufferEnd - inputBuffer, &bytesUsed);
        inputBuffer += bytesUsed;

        if (!codepoint)
        {
            if (bytesUsed) continue;
            else break;
        }

        (*outputBuffer++) = (byteSwap ? OSSwap32(codepoint) : codepoint);
    }

    if (usedSize) (*usedSize) = ((outputBuffer - outputBufferOriginal) * sizeof(CXCharUTF32));
    return (inputBuffer - inputBufferOriginal);
}

CXSize CXUTF16ToUTF32(CXCharUTF32 *outputBuffer, CXSize outputBufferSize, CXCharUTF16 *inputBuffer, CXSize inputBufferSize, CXSize *usedSize, OSBoolean byteSwap)
{
    outputBufferSize /= sizeof(CXCharUTF32);
    inputBufferSize  /= sizeof(CXCharUTF16);

    CXCharUTF32 *outputBufferOriginal = outputBuffer;
    CXCharUTF16 *inputBufferOriginal = inputBuffer;
    
    CXCharUTF32 *outputBufferEnd = outputBuffer + outputBufferSize;
    CXCharUTF16 *inputBufferEnd = inputBuffer + inputBufferSize;
    
    while ((outputBuffer < outputBufferEnd) && (inputBuffer < inputBufferEnd))
    {
        UInt8 bytesUsed;
        CXUnicodePoint codepoint = CXICodePointFromUTF16(inputBuffer, inputBufferEnd - inputBuffer, &bytesUsed, byteSwap);
        inputBuffer += bytesUsed;

        if (!codepoint)
        {
            if (bytesUsed) continue;
            else break;
        }

        (*outputBuffer++) = (byteSwap ? OSSwap32(codepoint) : codepoint);
    }

    if (usedSize) (*usedSize) = ((outputBuffer - outputBufferOriginal) * sizeof(CXCharUTF32));
    return (inputBuffer - inputBufferOriginal);
}

CXSize CXUTF32ToUTF32(CXCharUTF32 *outputBuffer, CXSize outputBufferSize, CXCharUTF32 *inputBuffer, CXSize inputBufferSize, CXSize *usedSize, OSBoolean byteSwap)
{
    CXSize bytesToCopy;
    
    if (inputBufferSize > outputBufferSize) {
        bytesToCopy = outputBufferSize;
    } else {
        bytesToCopy = inputBufferSize;
    }
    
    if (byteSwap) {
        memcpy(outputBuffer, inputBuffer, bytesToCopy);
    } else {
        memcpy(outputBuffer, inputBuffer, bytesToCopy);
    }
    
    if (usedSize) (*usedSize) = bytesToCopy;
    return (bytesToCopy * sizeof(CXCharUTF32));
}

CXSize CXUTF8BufferSizeForUTF8String(OSAddress inputBuffer, CXSize inputBufferSize)
{
    return inputBufferSize;
}

CXSize CXUTF16BufferSizeForUTF8String(CXCharUTF16 *inputBuffer, CXSize inputBufferSize, OSBoolean byteSwap)
{
    CXSize charsLeft = inputBufferSize / sizeof(CXCharUTF16);
    CXSize bufferSize = 0;

    while (charsLeft)
    {
        CXCharUTF16 character = (*inputBuffer++);
        if (byteSwap) character = OSSwap16(character);

        if (character >= kCXSurrogateHighBegin && character <= kCXSurrogateHighFinish) {
            bufferSize += 4;
            charsLeft -= 2;
            inputBuffer++;
        } else {
            if (character < 0x80) {
                bufferSize += 1;
            } else if (character < 0x800) {
                bufferSize += 2;
            } else {
                bufferSize += 3;
            }

            charsLeft--;
        }
    }

    return bufferSize;
}

CXSize CXUTF32BufferSizeForUTF8String(CXCharUTF32 *inputBuffer, CXSize inputBufferSize, OSBoolean byteSwap)
{
    CXSize charsLeft = inputBufferSize / sizeof(CXCharUTF32);
    CXSize bufferSize = 0;

    while (charsLeft)
    {
        CXCharUTF32 character = (*inputBuffer++);
        if (byteSwap) character = OSSwap32(character);
        charsLeft--;

        if (character < 0x80) {
            bufferSize += 1;
        } else if (character < 0x800) {
            bufferSize += 2;
        } else if (character < 0x10000) {
            bufferSize += 3;
        } else {
            bufferSize += 4;
        }
    }

    return bufferSize;
}

CXSize CXUTF8BufferSizeForUTF16String(CXCharUTF8 *inputBuffer, CXSize inputBufferSize)
{
    CXSize bytesLeft = inputBufferSize;
    CXSize bufferSize = 0;

    while (bytesLeft)
    {
        UInt8 bytes = kCXUTF8ExtraBytes[*inputBuffer++];
        if (--bytesLeft < bytes) break;

        bytesLeft -= bytes;
        inputBuffer += bytes;
        bufferSize += (bytes == 4 ? 2 : 1);
    }

    return (bufferSize * sizeof(CXCharUTF16));
}

CXSize CXUTF16BufferSizeForUTF16String(OSAddress inputBuffer, CXSize inputBufferSize)
{
    return inputBufferSize;
}

CXSize CXUTF32BufferSizeForUTF16String(CXCharUTF32 *inputBuffer, CXSize inputBufferSize, OSBoolean byteSwap)
{
    CXSize charsLeft = inputBufferSize / sizeof(CXCharUTF32);
    CXSize bufferSize = 0;

    while (charsLeft)
    {
        CXCharUTF32 character = (*inputBuffer++);
        if (byteSwap) OSSwap32(character);
        charsLeft--;

        if (character < 0x10000) {
            bufferSize += 1;
        } else {
            bufferSize += 2;
        }
    }

    return (bufferSize * sizeof(CXCharUTF16));
}

CXSize CXUTF8BufferSizeForUTF32String(CXCharUTF8 *inputBuffer, CXSize inputBufferSize)
{
    CXSize bytesLeft = inputBufferSize;
    CXSize bufferSize = 0;

    while (bytesLeft)
    {
        UInt8 bytes = kCXUTF8ExtraBytes[*inputBuffer++];
        if (--bytesLeft < bytes) break;

        inputBuffer += bytes;
        bytesLeft -= bytes;
        bufferSize++;
    }

    return (bufferSize * sizeof(CXCharUTF32));
}

CXSize CXUTF16BufferSizeForUTF32String(CXCharUTF16 *inputBuffer, CXSize inputBufferSize, OSBoolean byteSwap)
{
    CXSize charsLeft = inputBufferSize / sizeof(CXCharUTF16);
    CXSize bufferSize = 0;

    while (charsLeft)
    {
        CXCharUTF16 character = (*inputBuffer++);
        if (byteSwap) character = OSSwap16(character);
        bufferSize++;

        if (character >= kCXSurrogateHighBegin && character <= kCXSurrogateHighFinish)
        {
            inputBuffer++;
            charsLeft--;
        }
    }

    return (bufferSize * sizeof(CXCharUTF32));
}

CXSize CXUTF32BufferSizeForUTF32String(OSAddress inputBuffer, CXSize inputBufferSize)
{
    return inputBufferSize;
}

CXSize CXUTF8CountCodePoints(CXCharUTF8 *inputBuffer, CXSize inputBufferSize)
{
    return 0;
}

CXSize CXUTF16CountCodePoints(CXCharUTF16 *inputBuffer, CXSize inputBufferSize, OSBoolean byteSwap)
{
    return 0;
}

CXSize CXUTF32CountCodePoints(CXCharUTF32 *inputBuffer, CXSize inputBufferSize)
{
    return (inputBufferSize / sizeof(CXCharUTF32));
}

CXUnicodeConverter kCXUnicodeConverterUTF8 = {
    .toUTF8  = (OSFuncPtr)CXUTF8ToUTF8,
    .toUTF16 = (OSFuncPtr)CXUTF8ToUTF16,
    .toUTF32 = (OSFuncPtr)CXUTF8ToUTF32,

    .bufferSizeForUTF8String  = (OSFuncPtr)CXUTF8BufferSizeForUTF8String,
    .bufferSizeForUTF16String = (OSFuncPtr)CXUTF8BufferSizeForUTF16String,
    .bufferSizeForUTF32String = (OSFuncPtr)CXUTF8BufferSizeForUTF32String,

    .numberOfCodePoints = (OSFuncPtr)CXUTF8CountCodePoints,
    .isValidString = (OSFuncPtr)kOSNullPointer
};

CXUnicodeConverter kCXUnicodeConverterUTF16 = {
    .toUTF8  = (OSFuncPtr)CXUTF16ToUTF8,
    .toUTF16 = (OSFuncPtr)CXUTF16ToUTF16,
    .toUTF32 = (OSFuncPtr)CXUTF16ToUTF32,

    .bufferSizeForUTF8String  = (OSFuncPtr)CXUTF16BufferSizeForUTF8String,
    .bufferSizeForUTF16String = (OSFuncPtr)CXUTF16BufferSizeForUTF16String,
    .bufferSizeForUTF32String = (OSFuncPtr)CXUTF16BufferSizeForUTF32String,

    .numberOfCodePoints = (OSFuncPtr)CXUTF16CountCodePoints,
    .isValidString = (OSFuncPtr)kOSNullPointer
};

CXUnicodeConverter kCXUnicodeConverterUTF32 = {
    .toUTF8  = (OSFuncPtr)CXUTF32ToUTF8,
    .toUTF16 = (OSFuncPtr)CXUTF32ToUTF16,
    .toUTF32 = (OSFuncPtr)CXUTF32ToUTF32,

    .bufferSizeForUTF8String  = (OSFuncPtr)CXUTF32BufferSizeForUTF8String,
    .bufferSizeForUTF16String = (OSFuncPtr)CXUTF32BufferSizeForUTF16String,
    .bufferSizeForUTF32String = (OSFuncPtr)CXUTF32BufferSizeForUTF32String,

    .numberOfCodePoints = (OSFuncPtr)CXUTF32CountCodePoints,
    .isValidString = (OSFuncPtr)kOSNullPointer
};

CXUnicodeConverter CXUnicodeConverterForStringEncoding(CXStringEncoding encoding)
{
    switch (encoding)
    {
        case kCXStringEncodingUTF32: return kCXUnicodeConverterUTF32;
        case kCXStringEncodingUTF16: return kCXUnicodeConverterUTF16;
        case kCXStringEncodingUTF8:  return kCXUnicodeConverterUTF8;
    }

    CXFault();
    OSEndCode();
}

CXSize CXUnicodeConverterBufferSizeForEncoding(CXUnicodeConverter converter, CXStringEncoding encoding, OSAddress buffer, CXSize bufferSize, OSBoolean byteSwap)
{
    switch (encoding)
    {
        case kCXStringEncodingUTF32: return converter.bufferSizeForUTF32String(buffer, bufferSize, byteSwap);
        case kCXStringEncodingUTF16: return converter.bufferSizeForUTF16String(buffer, bufferSize, byteSwap);
        case kCXStringEncodingUTF8:  return converter.bufferSizeForUTF8String (buffer, bufferSize, byteSwap);
    }

    CXFault();
    OSEndCode();
}

CXSize CXUnicodeConverterToEncoding(CXUnicodeConverter converter, CXStringEncoding encoding, OSAddress outputBuffer, CXSize outputBufferSize, OSAddress inputBuffer, CXSize inputBufferSize, CXSize *usedSize, OSBoolean byteSwap)
{
    switch (encoding)
    {
        case kCXStringEncodingUTF32: return converter.toUTF32(outputBuffer, outputBufferSize, inputBuffer, inputBufferSize, usedSize, byteSwap);
        case kCXStringEncodingUTF16: return converter.toUTF16(outputBuffer, outputBufferSize, inputBuffer, inputBufferSize, usedSize, byteSwap);
        case kCXStringEncodingUTF8:  return converter.toUTF8 (outputBuffer, outputBufferSize, inputBuffer, inputBufferSize, usedSize, byteSwap);
    }

    CXFault();
    OSEndCode();
}
