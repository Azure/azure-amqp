#include "stdafx.h"
#include "MicroMock.h"
#include "MicroMockCharStarArenullTerminatedStrings.h"

bool operator==(_In_ const CMockValue<char*>& lhs, _In_ const CMockValue<char*>& rhs)
{
    if (lhs.GetValue() == NULL)
    {
        if (rhs.GetValue() == NULL)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (rhs.GetValue() == NULL)
        {
            return false;
        }
        else
        {
            return (strcmp(lhs.GetValue(), rhs.GetValue()) == 0);
        }
    }
}

 bool operator==(_In_ const CMockValue<const char*>& lhs, _In_ const CMockValue<const char*>& rhs)
{
    if (lhs.GetValue() == NULL)
    {
        if (rhs.GetValue() == NULL)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (rhs.GetValue() == NULL)
        {
            return false;
        }
        else
        {
            return (strcmp(lhs.GetValue(), rhs.GetValue()) == 0);
        }
    }
}