CREATE TABLE IF NOT EXISTS "UserClassMod" (
	"userID"		INT NOT NULL,
	"slot"			INT NOT NULL,
	"status"		VARCHAR(32),
	"sessionbonus"		VARCHAR(32),
	"displayinfo"		VARCHAR(32),
	"modbuff"		VARCHAR(32),
	"activeskill"		VARCHAR(32),
	"passiveskill"		VARCHAR(32),
	"addon"			VARCHAR(32),
	"pairingweapon"		VARCHAR(32),
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);