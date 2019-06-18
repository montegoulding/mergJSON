script "TestMergJSON"

local sRepoRoot

on TestSetup
	local tMode
	if $MODE is empty then
		put "Release" into tMode
	else
		put $MODE into tMode
	end if

	local tExtension, tSuffix, tPath
	switch the platform
	case "linux"
		put "so" into tExtension
		put "build" into tPath
		if the processor is "x86_64" then
			put "-x64" into tSuffix
		else
			put "-x86" into tSuffix
		end if
		break 
	case "macOS"
		put "bundle" into tExtension
		put "build/" & tMode into tPath
		break 
	case "win32"
		put "dll" into tExtension
		if the processor is "x86_64" then
			put "_build/" & tMode & "/x64" into tPath
			put "-x64" into tSuffix
		else
			put "_build/" & tMode & "/Win32" into tPath
			put "-x86" into tSuffix
		end if
		break 
	end switch

	set the itemDelimiter to slash
	put item 1 to -3 of the filename of me into sRepoRoot
	TestDiagnostic sRepoRoot & return & the filename of me

	_TestLoadExternal format("%s/%s/mergJSON%s.%s", \
		sRepoRoot, \
		tPath,\
		tSuffix, \
		tExtension)

	dispatch "revLoadLibrary" to \
		stack format("%s/mergJSON/mergJSONLibrary.livecodescript", \
			sRepoRoot)
end TestSetup

on _TestLoadExternal pExternal
	TestDiagnostic pExternal
	set the externals of the templateStack to pExternal

	create stack pExternal && "External"
	start using it
	if the externalCommands of it is empty and \
			the externalFunctions of it is empty then
		write "Cannot load external" && pExternal & return to stderr
		quit 1
	end if
end _TestLoadExternal

on TestTrivial
	local tJSON, tArray
	put "[true,false,true]" into tJSON
	put JSONToArray(tJSON) into tArray

	TestAssert "JSON Array creates 1..N indexes", tArray[3]
end TestTrivial

on TestJSONTestSuite
	local tSuiteFolder
	put format("%s/livecode/extensions/libraries/json/tests/JSONTestSuite", \
			sRepoRoot) into tSuiteFolder

	attemptLoad format("%s/tests/broken.json", sRepoRoot)
	attemptParse it

	local tBroken
	put it into tBroken

	local tFilename, tDescription, tSuccess
	local tSkipReason, tBrokenReason, tResult

	repeat for each line tFilename in files(tSuiteFolder & "/test_parsing")
		put getDescription(tFilename) into tDescription
		TestDiagnostic merge("Processing [[tFilename]]")

		put true into tSuccess
		if tSuccess then
			attemptLoad format("%s/test_parsing/%s", tSuiteFolder, tFilename)
			put the result into tResult
			if tResult is not empty then
				put false into tSuccess
				TestDiagnostic tResult
			end if
		end if

		if tSuccess then
			attemptParse it
			put the result into tResult
			if tResult is not empty then
				put false into tSuccess
				TestDiagnostic tResult
			else
				TestDiagnostic "Parsed successfully"
			end if
		end if

		put tBroken[tFilename] into tBrokenReason

		put (tSuccess and isSupposedToParse(tFilename)) or \
				(not tSuccess and not isSupposedToParse(tFilename)) \
				into tSuccess

		if tBrokenReason is not empty then
			TestAssertBroken tDescription, tSuccess, tBrokenReason
		else
			TestAssert tDescription, tSuccess
		end if
	end repeat
end TestJSONTestSuite

private command attemptLoad pFilename
	local tText
	local tUrlResult
	put url ("binfile:" & pFilename) into tText
	put the result into tUrlResult
	if tUrlResult is not empty then
		TestDiagnostic merge("Failed to load [[pFilename]]: [[tUrlResult]]")
		return "read failed" for error
	end if

	return tText for value
end attemptLoad

private command attemptParse pJson
	local tError
	try
		return JSONToArray(pJson) for value
	catch tError
		return tError for error
	end try
end attemptParse

private function getDescription pFilename
	put char 3 to -6 of pFilename into pFilename
	replace "_" with " " in pFilename
	replace "#" with "U+0023" in pFilename
	return pFilename
end getDescription

private function isSupposedToParse pFilename
	switch char 1 of pFilename
		case "y"
			return true
		case "n"
			return false
	end switch
end isSupposedToParse
