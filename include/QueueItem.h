/**
  * The source code in this file is the property of
  * Ripple Systems and is not for redistribution
  * in any form.
  *
  * Source:   $File: //depot/3001_TIP_NEW/TA_BASE/transactive/core/threads/src/QueueItem.h $
  * @author:  Ripple
  * @version: $Revision: #2 $
  *
  * Last modification: $DateTime: 2012/02/06 16:15:14 $
  * Last modified by:  $Author: haijun.li $
  *
  * The QueueProcessor thread is responsible for processing queue items
  * using a pure virtual method provided by the subclass
  *
  */

#ifndef QUEUEITEM_H
#define QUEUEITEM_H

#include "boost/shared_ptr.hpp"

const unsigned long MAX_QUEUEPROCESSOR_QUEUE_SIZE = 1000000;

// the queue holds pointers
// we want to insert unique, we dont want to insert unique
// pointers, but unique items, so we must define a structure so
// an operator== that compares items rather than pointers can be used
template<class ITEM> class QueueItem
{

protected:

	boost::shared_ptr<ITEM> m_ptr;

public:

	QueueItem ( boost::shared_ptr<ITEM> itemPtr )
	{
		m_ptr = itemPtr;
	}

	bool operator== ( const boost::shared_ptr<ITEM>& rhs ) const
	{
		return *m_ptr == *rhs;
	}

	bool operator== ( const QueueItem& rhs ) const
	{
		return *m_ptr == *(rhs.getItemPtr());
	}

	bool isReplacedBy ( const boost::shared_ptr<ITEM>& replacePtr ) const
	{
		return m_ptr->isReplacedBy(replacePtr);
	}

	bool isReplacedBy ( const QueueItem& replacePtr ) const
	{
		return m_ptr->isReplacedBy(replacePtr.getItemPtr());
	}

	const boost::shared_ptr<ITEM>& getItemPtr() const
	{
		return m_ptr;
	}
};


#endif
