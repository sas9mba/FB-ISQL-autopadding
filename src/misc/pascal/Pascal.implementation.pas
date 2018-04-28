constructor FbException.create(status: IStatus);
begin
	inherited Create('FbException');
	self.status := status.clone;
end;

destructor FbException.Destroy();
begin
	status.dispose;
	inherited Destroy;
end;

function FbException.getStatus: IStatus;
begin
	Result := status;
end;

class procedure FbException.checkException(status: IStatus);
begin
	if ((status.getState and status.STATE_ERRORS) <> 0) then
		raise FbException.create(status);
end;

class procedure FbException.catchException(status: IStatus; e: Exception);
var
	statusVector: array[0..4] of NativeIntPtr;
	msg: AnsiString;
begin
	if (e.inheritsFrom(FbException)) then
		status.setErrors(FbException(e).getStatus.getErrors)
	else
	begin
		msg := e.message;

		statusVector[0] := NativeIntPtr(isc_arg_gds);
		statusVector[1] := NativeIntPtr(isc_random);
		statusVector[2] := NativeIntPtr(isc_arg_string);
		statusVector[3] := NativeIntPtr(PAnsiChar(msg));
		statusVector[4] := NativeIntPtr(isc_arg_end);

		status.setErrors(@statusVector);
	end
end;
