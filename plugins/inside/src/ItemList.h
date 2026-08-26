#pragma once
#include "all_far.h"

struct FP_ItemList
{
		PluginPanelItem *List;         ///< Panel items array
		int              ItemsCount;   ///< Number of items in array
		BOOL             needToDelete; ///< Indicate if local copy of items array need to be deleted at destructor
		int              MaxCount;     ///< Number of elements may be inserted into list without expand list
	private:
		BOOL Realloc(int DeltaSize);
	public:
		FP_ItemList(BOOL NeedToDelete = TRUE);
		~FP_ItemList() { Clear(); }

		PluginPanelItem *Add(const PluginPanelItem *src,int cn);                            ///<Add a `cn` items to list
		PluginPanelItem *Add(const PluginPanelItem *src)  { return Add(src,1); }
		void             Clear(void);                                                       //Clear list

		PluginPanelItem *Items(void)                      { return List; }
		int              Count(void)                      { return ItemsCount; }
		PluginPanelItem *Item(int num)                    { return (num >= 0 && num < ItemsCount) ? (List+num) : NULL; }
		PluginPanelItem *operator[](int num)              { return Item(num); }

		static void      Free(PluginPanelItem *List,int count);                            //Free items, allocated by FP_ItemList
		static void      Copy(PluginPanelItem *dest,const PluginPanelItem *src,int cn);    //Copy all data from `src` to `dest`
};

struct FP_SizeItemList: public FP_ItemList
{
		int64_t TotalFullSize;
		int64_t TotalFiles;
	public:
		FP_SizeItemList(BOOL NeedToDelete = TRUE) : FP_ItemList(NeedToDelete)
		{
			TotalFullSize = 0;
			TotalFiles    = 0;
		}
};
