%address = "192.168.1.24"; % use this for wifi
address = '192.168.7.2';
port = 1045;

% Create a UDP object
%udpObject = udpport(address, port, 'LocalPort', 49152);
udpObject = udpport("datagram", "IPV4");


% Send data to the remote host
for x = 0:0.1:2*pi
    y = sin(x);

    % Convert the data to bytes before sending

    %fwrite(udpObject, data, 'uint8');
    a = 1:11;
    write(udpObject,y,"double","192.168.7.2",1045);

    pause(1);
end

% Close the UDP object
fclose(udpObject);

% Delete the UDP object
delete(udpObject);



