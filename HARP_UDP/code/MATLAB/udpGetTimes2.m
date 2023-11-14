% udpGetTimes1.m
%
% testing program for getting data from 4 ch HARP 3B04 230307
% two channels at 200kHz/ch
% UDP 1 packet = 1252 bytes = 12 bytes time header + 1240 bytes data
% 1 datagram = 1 packet
%
% based on udpGetTimes1.m
%
% 230312 smw

clearvars

hsz = 12;           % packet head size (bytes)
nchpp = 2;          % number of channels per packet
sppch = 5*62;       % samples per packet per channel = 310
bps = 2;            % bytes per sample
dsz = sppch * nchpp * bps;         % packet data size (bytes) = 1240
psz = hsz + dsz;    % packet size (bytes) = 1252

blkinterval = 1550; % block/packet/datagram size microseconds = 1e6 * sppch/200e3

% Create a udpport object udpportObj that uses IPV4 and communicates in byte mode. The
% object is bound to the local host at "192.168.100.100" and the local port 50000 with
% port sharing disabled.
udpportObj = udpport("LocalHost","192.168.100.100","LocalPort",50000);

% need 100 bytes to get Open command through
% Write the data ['Open',zeros(1,96)] as a uint8 using the udpport object udpportObj to the device with
% address or host name "192.168.100.220" and port 50000.
write(udpportObj,['Open',zeros(1,96)],"uint8","192.168.100.220",50000);

fprintf('UDP from HARP, check timestamps\n')
pcnt = 0;
lcnt = 0;
flag1 = 1;
while(1)
    %     flush(udpportObj,"output");
    % 1 packet = 1252 bytes = 12 bytes time header + 1240 bytes data
    % Read values of uint8 data using the udpport object udpportObj.
    data1 = read(udpportObj,psz,"uint8");
    pcnt = pcnt + 1;
    time1 = data1(1:6);
    usec = int32(swapbytes(typecast(uint8(data1(7:10)),'uint32')));  % microseconds (delta = 1550 usec)
    if flag1 == 1
        usec0 = usec;
        flag1 = 2;
        pcnt = pcnt + 1;
        fprintf("First Time: %02d/%02d/%02d %02d:%02d:%02d.%06d  %06d\n", ...
            time1(2), time1(3), time1(1), time1(4:6), usec);
        fprintf("\n");
        continue
    else
        dusec = usec - usec0;
        if dusec < 0
            dusec = dusec + 1e6;
        end
    end
    if dusec ~= blkinterval
        fprintf("Time Glitch: %02d/%02d/%02d %02d:%02d:%02d.%06d  %06d\n", ...
            time1(2), time1(3), time1(1), time1(4:6), usec, dusec);
        if dusec == 0  % unlikely error, but still not acceptable
            fprintf('Error1: Close socket and try again\n')
            write(udpportObj,['Close',zeros(1,95)],"uint8","192.168.100.220",50000);
            udpGetTimes2
        end
        if abs(usec) > 1e6  % cannot have usec that are that large
            fprintf('Error2: Close socket and try again\n')
            write(udpportObj,['Close',zeros(1,95)],"uint8","192.168.100.220",50000);
            udpGetTimes2
        end
        if time1(3) == 0 || time1(1) == 0  % cannot have months or days = 0
            fprintf('Error3: Close socket and try again\n')
            write(udpportObj,['Close',zeros(1,95)],"uint8","192.168.100.220",50000);
            udpGetTimes2
        end
    end

    usec0 = usec;

    if pcnt >= 1000
        pcnt = 0;
        fprintf(".");
        lcnt = lcnt + 1;
        if lcnt >= 50
            lcnt = 0;
            fprintf("Time: %02d/%02d/%02d %02d:%02d:%02d.%06d  %06d\n", ...
                time1(2), time1(3), time1(1), time1(4:6), usec);
            fprintf("\n");
        end
    end
    %     data2  = swapbytes(typecast(uint8(data1(13:psz)),'uint16'));
end


% close the connection
write(udpportObj,['Close',zeros(1,95)],"uint8","192.168.100.220",50000);
