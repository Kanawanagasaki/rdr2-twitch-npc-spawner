namespace Rdr2.TwitchNpcSpawner.Services;

using Rdr2.TwitchNpcSpawner.Entities;
using Microsoft.EntityFrameworkCore;

public class MySqlContext : DbContext
{
    public required DbSet<UserEntity> Users { get; init; }
    public required DbSet<RewardEntity> Rewards { get; init; }
    public required DbSet<HeartBeatEntity> HeartBeats { get; init; }

    private IConfiguration _config;

    public MySqlContext(IConfiguration config, ILogger<MySqlContext> logger)
    {
        _config = config;

        try
        {
            Database.Migrate();
        }
        catch (Exception e)
        {
            logger.LogWarning("Exception was thrown during migration:\n" + e.Message);
        }
    }

    protected override void OnConfiguring(DbContextOptionsBuilder optionsBuilder)
    {
        optionsBuilder.UseMySql(
            $"server={_config["MySql:Host"]};" +
            $"user={_config["MySql:User"]};" +
            $"password={_config["MySql:Password"]};" +
            $"database={_config["MySql:Database"]}",
            new MySqlServerVersion(_config["MySql:Version"])
        );
    }
}