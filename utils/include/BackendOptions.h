struct BackendOptions {
	BOOL UseModernLook;     	// unicode glyphs for check boxes / radio buttons
	BOOL UseRoundedBorders; 	// wx only: rounded corners for borders
	BOOL UseSingleBordersOnly;	// wx only: replace double borders to single
	BOOL UseNoBorders;			// wx only: replace double borders to single
	BOOL UseEmbossAsBold;		// wx only: repplace bold glyphs to emboss effect
	BOOL UseSoftenBevels;		// wx only: makes boxes less bright for black and white case
	BOOL Use3D;                 // wx only: 3d buttons
};
