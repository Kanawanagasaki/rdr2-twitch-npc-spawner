namespace Rdr2.TwitchNpcSpawner.Client.Data;

public class EventSub
{
    public string? state { get; init; }
    public long lastMessageMs { get; init; }
    public long nextRedemptionPull { get; init; }
    public long lastRedemptionPull { get; init; }
}
