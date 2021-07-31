﻿/// \file
/// \author 
///
/// \brief Implementation of delay snippet fragment class.
///  
/// Copyright (c) . All rights reserved.  
/// Licensed under the MIT License. See LICENSE file in the project root for full license information. 


#include "stdafx.h"
#include "DelaySnippetFragment.h"


//**********************************************************************************************************************
/// \param[in] delayMs The delay in milliseconds.
//**********************************************************************************************************************
DelaySnippetFragment::DelaySnippetFragment(qint32 delayMs)
   : SnippetFragment()
   , delayMs_(delayMs)
{
}


//**********************************************************************************************************************
/// \return The type of the snippet fragment.
//**********************************************************************************************************************
SnippetFragment::EType DelaySnippetFragment::type() const
{
   return EType::Delay;
}


//**********************************************************************************************************************
//
//**********************************************************************************************************************
void DelaySnippetFragment::render() const
{
   QThread::msleep(delayMs_);
}